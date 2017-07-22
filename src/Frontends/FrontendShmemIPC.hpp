#ifndef SIGIL2_FRONTEND_SHMEM_IPC_H
#define SIGIL2_FRONTEND_SHMEM_IPC_H

#include "Core/SigiLog.hpp"
#include "Core/Frontends.hpp"
#include "CommonShmemIPC.h"
#include "Common.hpp"
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>

/**
 * Sigil2 receives events from external tools via an array of
 * shared memory buffers. The external tools fill the buffers as a
 * forked process. See CommonShmemIPC.h for details of these buffers.
 *
 * There should exist at least 2 buffers, so the external tool can fill a buffer
 * while Sigil2 reads from the other buffer.
 * The optimal amount of buffering done depends on the system resources
 * and the variation in the event processing backend.
 *
 * Named pipes are used for syncing buffering between Sigil2 and the external
 * tool. This allows each process to block when they cannot proceed,
 * as in the case of no empty buffers to write to (external tool)
 * or no full buffers to read from (Sigil2 backend).
 * Otherwise each process would require less efficient synchronization methods
 * such as spinning.
 *
 * XXX The term 'full' buffer is for historical reasons. A buffer does not
 * necessarily have to be full when used by Sigil2. There should be metadata
 * available to let Sigil2 know how many valid events are in the buffer.
 */


using SigiLog::warn;
using SigiLog::fatal;

namespace Cleanup
{
auto setCleanupDir(std::string dir) -> void;
}; //end namespace Cleanup


template <typename SharedData>
class ShmemFrontend : public FrontendIface
{
    const std::string ipcDir;
    const std::string emptyFifoName;
    const std::string fullFifoName;
    const std::string shmemName;
    int emptyfd;
    int fullfd;
    FILE *shmemfp;
    SharedData *shmem;

    /* IPC configuration */
    CircularQueue<int, SIGIL2_IPC_BUFFERS> q;
    Sem filled{0}, emptied{SIGIL2_IPC_BUFFERS};
    int lastBufferIdx;
    /* Keep track of which buffers are in use/ready */

    std::thread eventLoop;
    /* Asynchronously manage external events */

  public:
    ShmemFrontend(const std::string &ipcDir)
        : ipcDir       (ipcDir)
        , emptyFifoName(ipcDir + "/" + SIGIL2_IPC_EMPTYFIFO_BASENAME + "-" + std::to_string(uid))
        , fullFifoName (ipcDir + "/" + SIGIL2_IPC_FULLFIFO_BASENAME  + "-" + std::to_string(uid))
        , shmemName    (ipcDir + "/" + SIGIL2_IPC_SHMEM_BASENAME     + "-" + std::to_string(uid))
    {
        initShMem();
        emptyfd = createAndOpenNewFifo(emptyFifoName.c_str(), O_WRONLY);
        fullfd = createAndOpenNewFifo(fullFifoName.c_str(), O_RDONLY);

        /* asynchronously manage communications with the external tool */
        eventLoop = std::thread{&ShmemFrontend::receiveEventsLoop, this};

        FrontendIface::nameBase = [&]{ assert(lastBufferIdx < decltype(lastBufferIdx){SIGIL2_IPC_BUFFERS});
                                       return shmem->nameBuffers[lastBufferIdx].names; };
    }

    ~ShmemFrontend() override
    {
        /* All communication with the external tool
         * should be completed by destruction */
        eventLoop.join();
        disconnect();
    }

    virtual auto acquireBuffer() -> EventBufferPtr override final
    {
        filled.P();
        lastBufferIdx = q.dequeue();

        /* can be negative to signal the end of the event stream */
        assert(lastBufferIdx < decltype(lastBufferIdx){SIGIL2_IPC_BUFFERS});

        if (lastBufferIdx < 0)
            return nullptr;
        else
            return EventBufferPtr(&(shmem->eventBuffers[lastBufferIdx]));
    }

    virtual auto releaseBuffer(EventBufferPtr eventBuffer) -> void override final
    {
        eventBuffer.release();
        emptied.V();

        /* Tell Valgrind that the buffer is empty again */
        assert(lastBufferIdx < decltype(lastBufferIdx){SIGIL2_IPC_BUFFERS} && lastBufferIdx >= 0);
        writeEmptyFifo(lastBufferIdx);
    }


  private:
    auto initShMem() -> void
    {
        /* Initialize IPC between Sigil2 and the external tool */

        shmemfp = fopen(shmemName.c_str(), "wb+");
        if (shmemfp == nullptr)
            fatal(std::string("sigil2 shared memory file open failed -- ") + strerror(errno));

        /* XXX From write(2) man pages:
         *
         * On Linux, write() (and similar system calls) will transfer at most
         * 0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
         * actually transferred.  (This is true on both 32-bit and 64-bit
         * systems.)
         *
         * fwrite doesn't have this limitation */
        auto init = std::make_unique<SharedData>();
        int count = fwrite(init.get(), sizeof(SharedData), 1, shmemfp);
        if (count != 1)
        {
            fclose(shmemfp);
            fatal(std::string("sigil2 shared memory file write failed -- ") + strerror(errno));
        }

        shmem = reinterpret_cast<SharedData *>
            (mmap(nullptr, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED,
                  fileno(shmemfp), 0));
        if (shmem == MAP_FAILED)
        {
            fclose(shmemfp);
            fatal(std::string("sigil2 mmap shared memory failed -- ") + strerror(errno));
        }
    }

    auto createAndOpenNewFifo(const char *path, int flags) const -> int
    {
        if (mkfifo(path, 0600) < 0)
        {
            if (errno == EEXIST)
            {
                if (remove(path) != 0)
                    fatal(std::string("sigil2 could not delete old fifos -- ") + strerror(errno));

                if (mkfifo(path, 0600) < 0)
                    fatal(std::string("sigil2 failed to create fifos -- ") + strerror(errno));
            }
            else
                fatal(std::string("sigil2 failed to create fifos -- ") + strerror(errno));
        }

        int fd = open(path, flags);
        if (fd < 0)
            fatal(std::string("sigil2 failed to open fifo: ") + path + " -- " + strerror(errno));

        return fd;
    }

    auto disconnect() -> void
    {
        munmap(shmem, sizeof(SharedData));
        fclose(shmemfp);
        close(emptyfd);
        close(fullfd);
    }

    auto readFullFifo() -> int
    {
        /* Reads from 'full' fifo and returns the data.
         * This is an index to the buffer array in the shared memory.
         * It implicitly informs Sigil2 that buffer[idx] has
         * been filled with events and can be consumed by
         * the Sigil2 backend. */

        int full_data;
        int res = read(fullfd, &full_data, sizeof(full_data));

        if (res == 0)
            fatal("Unexpected end of fifo");
        else if (res < 0)
            fatal(std::string("could not read from full-fifo -- ") + strerror(errno));

        return full_data;
    }

    auto writeEmptyFifo(unsigned idx) -> void
    {
        /* The 'idx' sent informs the external tool that buffer[idx]
         * has been consumed by the Sigil2 backend,
         * and that the external tool can now fill it with events again. */

        int res = write(emptyfd, &idx, sizeof(idx));
        if (res < 0)
            fatal(std::string("could not send empty buffer status -- ") + strerror(errno));
    }

    auto receiveEventsLoop() -> void
    {
        /* main event loop for managing the event buffers */

        bool finished = false;
        while (finished == false)
        {
            /* external tool sends event buffer metadata */
            unsigned fromTool = readFullFifo();
            emptied.P();

            if (fromTool == SIGIL2_IPC_FINISHED)
            {
                finished = true;
            }
            else
            {
                assert(fromTool < decltype(fromTool){SIGIL2_IPC_BUFFERS} &&
                       fromTool >= 0);
                q.enqueue(fromTool);
                filled.V();
            }
        }

        /* Signal the end of the event stream */
        q.enqueue(-1);
        filled.V();
    }
};

#endif

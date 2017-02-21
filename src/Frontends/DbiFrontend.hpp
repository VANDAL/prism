#ifndef SGL_FRONTEND_DBI_H
#define SGL_FRONTEND_DBI_H

#include "Sigil2/Frontends.hpp"
#include "DbiIpcCommon.h"
#include "Common.hpp"
#include "Sigil2/SigiLog.hpp"
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>

/**
 * Sigil2 receives events from a DBI tool via a set of shared memory buffers
 * that are filled by the DBI tool in a forked process. See DbiIpcCommon.h
 * for details of these buffers.
 *
 * There should exist at least 2 buffers, so Valgrind can fill a buffer
 * with events while Sigil2 reads from the other buffer. The optimal amount
 * of buffering done depends on the system resources and the variation in
 * event processing in the backend.
 *
 * A set of named pipes is used for syncing buffering between Sigil2 and
 * the DBI tool. This allows each process to block when they cannot proceed,
 * as in the case of no empty buffers to write to (DBI tool) or no full buffers
 * to read from (Sigil2 backend).
 * Otherwise each process would require less efficient synchronization methods
 * such as spinning.
 */

using SigiLog::warn;
using SigiLog::fatal;

class DBIFrontend : public FrontendIface
{
    /* IPC configuration */
    const std::string ipcDir;
    const std::string shmemName;
    const std::string emptyFifoName;
    const std::string fullFifoName;
    int emptyfd;
    int fullfd;
    Sigil2DBISharedData *shmem;

    /* Keep track of which buffers are in use/ready */
    CircularQueue<int, SIGIL2_DBI_BUFFERS> q;
    Sem filled{0}, emptied{SIGIL2_DBI_BUFFERS};
    int lastBufferIdx;

    /* Manage DBI events asynchronously */
    std::shared_ptr<std::thread> eventLoop;

  public:
    DBIFrontend(const std::string &ipcDir)
        : ipcDir       (ipcDir)
        , shmemName    (ipcDir + "/" + SIGIL2_DBI_SHMEM_NAME)
        , emptyFifoName(ipcDir + "/" + SIGIL2_DBI_EMPTYFIFO_NAME)
        , fullFifoName (ipcDir + "/" + SIGIL2_DBI_FULLFIFO_NAME)
    {
        initShMem();
        makeNewFifo(emptyFifoName.c_str());
        makeNewFifo(fullFifoName.c_str());
        connectDBI();

        /* asynchronously manage communications with the DBI tool */
        eventLoop = std::make_shared<std::thread>(&DBIFrontend::receiveEventsLoop, this);
    }

    ~DBIFrontend()
    {
        /* All communication with the DBI tool
         * should be completed by destruction */
        eventLoop->join();
        disconnectDBI();
        fileCleanup();
    }

    virtual auto acquireBuffer() -> EventBuffer* override
    {
        filled.P();
        lastBufferIdx = q.dequeue();

        /* can be negative to signal the end of the event stream */
        assert(lastBufferIdx < decltype(lastBufferIdx){SIGIL2_DBI_BUFFERS});

        if (lastBufferIdx < 0)
            return nullptr;
        else
            return &(shmem->buf[lastBufferIdx]);
    }

    virtual auto releaseBuffer() -> void override
    {
        emptied.V();

        /* Tell Valgrind that the buffer is empty again */
        assert(lastBufferIdx < decltype(lastBufferIdx){SIGIL2_DBI_BUFFERS} && lastBufferIdx >= 0);
        writeEmptyFifo(lastBufferIdx);
    }


  private:
    /* Initialize IPC between Sigil2 and the DBI tool */
    auto initShMem() -> void
    {
        FILE *fd = fopen(shmemName.c_str(), "wb+");
        if (fd == nullptr)
            fatal(std::string("sigil2 shared memory file open failed -- ") + strerror(errno));

        /* XXX From write(2) man pages:
         *
         * On Linux, write() (and similar system calls) will transfer at most
         * 0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
         * actually transferred.  (This is true on both 32-bit and 64-bit
         * systems.)
         *
         * fwrite doesn't have this limitation */
        std::unique_ptr<Sigil2DBISharedData> init(new Sigil2DBISharedData());
        int count = fwrite(init.get(), sizeof(Sigil2DBISharedData), 1, fd);
        if (count != 1)
        {
            fclose(fd);
            fatal(std::string("sigil2 shared memory file write failed -- ") + strerror(errno));
        }

        shmem = reinterpret_cast<Sigil2DBISharedData *>
            (mmap(nullptr, sizeof(Sigil2DBISharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fd), 0));
        if (shmem == MAP_FAILED)
        {
            fclose(fd);
            fatal(std::string("sigil2 mmap shared memory failed -- ") + strerror(errno));
        }

        fclose(fd);
    }

    auto makeNewFifo(const char *path) const -> void
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
    }

    auto connectDBI() -> void
    {
        /* XXX Sigil2 might get stuck blocking if the DBI tool
         * unexpectedly exits before connecting at this point */

        emptyfd = open(emptyFifoName.c_str(), O_WRONLY);
        if (emptyfd < 0)
            fatal(std::string("sigil2 failed to open empty-fifo for writing -- ") + strerror(errno));

        fullfd = open(fullFifoName.c_str(), O_RDONLY);
        if (fullfd < 0)
            fatal(std::string("sigil2 failed to open full-fifo for reading -- ") + strerror(errno));
    }

    auto disconnectDBI() -> void
    {
        munmap(shmem, sizeof(Sigil2DBISharedData));
        close(emptyfd);
        close(fullfd);
    }

    auto fileCleanup() -> void
    {
        if (remove(shmemName.c_str()) != 0 ||
            remove(emptyFifoName.c_str()) != 0 ||
            remove(fullFifoName.c_str())  != 0 ||
            remove(ipcDir.c_str())    != 0)
            warn(std::string("error deleting IPC files -- ") + strerror(errno));
    }

    /* Reads an int from 'full' fifo and returns the data.
     * This is an index to the buffer array in the shared memory.
     * It informs Sigil2 that buffer[idx] has been filled with
     * events and can be consumed by the Sigil2 backend. */
    auto readFullFifo() -> int
    {
        int full_data;
        int res = read(fullfd, &full_data, sizeof(full_data));

        if (res == 0)
            fatal("Unexpected end of fifo");
        else if (res < 0)
            fatal(std::string("could not read from full-fifo -- ") + strerror(errno));

        return full_data;
    }

    /* The 'idx' sent informs the DBI tool that buffer[idx]
     * has been consumed by the Sigil2 backend,
     * and that the DBI tool can now fill it with events again. */
    auto writeEmptyFifo(unsigned idx) -> void
    {
        auto res = write(emptyfd, &idx, sizeof(idx));
        if (res < 0)
            fatal(std::string("could not send empty buffer status -- ") + strerror(errno));
    }

    /* main event loop for managing the event buffers */
    auto receiveEventsLoop() -> void
    {
        bool finished = false;
        while (finished == false)
        {
            /* DBI tool sends event buffer metadata */
            unsigned fromDBI = readFullFifo();
            unsigned idx;
            emptied.P();

            if (fromDBI == SIGIL2_DBI_FINISHED)
            {
                finished = true;
                idx = readFullFifo();
            }
            else
            {
                idx = fromDBI;
            }

            assert(idx < decltype(idx){SIGIL2_DBI_BUFFERS} && idx >= 0);
            q.enqueue(idx);
            filled.V();
        }

        /* Signal the end of the event stream */
        q.enqueue(-1);
        filled.V();
    }
};

#endif

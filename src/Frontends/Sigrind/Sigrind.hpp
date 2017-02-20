#ifndef SGL_SIGRIND_H
#define SGL_SIGRIND_H

#include "Sigil2/Frontends.hpp"
#include "SigrindIPC.h"

#include <condition_variable>
#include <atomic>
#include <thread>

/**
 * Sigil2 receives events from Valgrind via a set of shared memory buffers
 * There should exist at least 2 buffers, so Valgrind can fill a buffer
 * with events while Sigil2 reads from the other buffer. The optimal amount
 * of buffering done depends on the system resources and the variation in
 * event processing in the backend.
 *
 * A set of named pipes is used for syncing buffering between Sigil2 and
 * Valgrind. This allows each process to block when they cannot proceed,
 * compared to other IPC synchronization methods such as spinning.
 *
 * Because Valgrind serializes multithreaded applications (only allowing
 * one thread to run at a time), Sigil2 treats Valgrind as single-threaded.
 * Only one set of named pipes and buffers is necessary, for the one Valgrind
 * thread.
 *
 * The Valgrind tool, Sigrind, will send a special event marker (thread swap)
 * to let Sigil2 know that the event stream switched to a new thread.
 */

namespace sgl
{

/* Required by Sigil2 to begin event generation.
 * Each frontend requires this function signature.
 * Valgrind will be forked off as a separate process
 * and begin generating events to send to Sigil2. */
auto startSigrind(FrontendStarterArgs args) -> FrontendIfaceGenerator;


class Sem
{
  public:
    Sem(int init) : val(init) {}

    auto P() -> void
    {
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, [&] { return val > 0; });
        --val;
    }

    auto V() -> void
    {
        std::unique_lock<std::mutex> lock(mtx);
        ++val;
        cond.notify_one();
    }

    auto value() -> int
    {
        std::unique_lock<std::mutex> lock(mtx);
        return val;
    }

  private:
    std::mutex mtx;
    std::condition_variable cond;
    int val;
};

/* For shared memory resource control */
template<typename T, unsigned N>
struct CircularQueue
{
    static_assert((N >= 2) && ((N & (N - 1)) == 0), "N must be a power of 2");
  public:
    auto enqueue(T val) -> void
    {
        q[tail] = val;
        tail = (tail+1) & (N-1);
    }

    auto dequeue() -> T
    {
        auto temp = head;
        head = (head+1) & (N-1);
        return q[temp];
    }

    T q[N];
    size_t head{0}, tail{0};
};


class Sigrind : public FrontendIface
{
    /* Handle interfacing the shared memory buffers to the Sigil2 core */
    using ResourceQueue = CircularQueue<int, NUM_BUFFERS>;

  public:
    Sigrind(const std::string &ipcDir);
    ~Sigrind();
    virtual auto acquireBuffer() -> EventBuffer* override;
    virtual auto releaseBuffer() -> void override;


  private:
    /* Clean up actions if an unexpected quit happens */
    auto setInterruptOrTermHandler() -> void;

    /* Initialize IPC between Sigil2 and Valgrind */
    auto initShMem()                         -> void;
    auto makeNewFifo(const char *path) const -> void;
    auto connectValgrind()                   -> void;

    /* Reads from 'full' fifo and returns the data.
     * This is an index to the buffer array in the shared memory.
     * It informs Sigil2 that buffer[idx] has been filled with
     * events and can be passed to the Sigil2 event manager/buffers. */
    auto readFullFifo() -> int;

    /* Write param to 'empty' fifo.
     * This is the index to the buffer array in the shared memory.
     * It informs valgrind that buffer[idx] has been emptied by
     * Sigil2. Valgrind can then take ownership of that buffer and fill
     * it with events */
    auto writeEmptyFifo(unsigned idx) -> void;

    /* main event loop for managing event buffer */
    auto receiveValgrindEventsLoop() -> void;
    std::shared_ptr<std::thread> eventLoop;

    const std::string ipcDir;
    const std::string shmemName;
    const std::string emptyFifoName;
    const std::string fullFifoName;

    /* IPC */
    int emptyfd;
    int fullfd;
    SigrindSharedData *shared;

    ResourceQueue q;
    Sem filled{0}, emptied{NUM_BUFFERS};
    int lastBufferIdx;
};

}; //end namespace sgl

#endif

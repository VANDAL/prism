#ifndef SGL_EVENTBUFFER_H
#define SGL_EVENTBUFFER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <cassert>

#include "Primitive.h"
#include "Backends.hpp"

namespace sgl
{

/* fudge numbers */
constexpr unsigned int MAX_EVENTS = 100000;
constexpr unsigned int MAX_BUFFERS = 8; // XXX MUST be a power of 2 and > 1

class EventBuffer
{
  public:
    template<typename T> using EventAction = std::function<void(T)>;

    EventBuffer();
    ~EventBuffer();

    template<typename T>
    void addEvent(const T &ev);

    struct Buffer
    {
        BufferedSglEv events[MAX_EVENTS];
        size_t used = 0;
    };

    /* Flush the next available sub-buffer with backend */
    void flush(Backend &backend);

    /* Signal that the partially
     * full sub-buffer can be consumed  */
    void complete();

    /* Return if all sub-buffers have been consumed.
     * Because at least one sub-buffer is always not empty,
     * which is set during construction, this should
     * never return true until 'complete()' is called */
    bool isEmpty();

  private:
    /* Simple semaphore implementation */
    class Sem
    {
      public:
        Sem(int init) : count(count_), count_(init) {}
        int P()
        {
            std::unique_lock<std::mutex> ulock(mut);
            cond.wait(ulock, [&] { return count > 0; });
            --count_;
            return count_;
        }
        int V()
        {
            std::unique_lock<std::mutex> ulock(mut);
            ++count_;
            cond.notify_one();
            return count_;
        }

        const unsigned int &count;
      private:
        unsigned int count_;
        std::mutex mut;
        std::condition_variable cond;
    };

    /* Circular counter to help indexing if more buffers
     * are decided in the future. */
    struct CircularCounter
    {
        CircularCounter(int mod_val) : mod_mask(mod_val - 1)
        {
            /* must be a power of 2 */
            assert((mod_val >= 2) && ((mod_val & (mod_val - 1)) == 0));
            val = 0;
        }
        int increment()
        {
            val = (val + 1) & mod_mask;
            return val;
        }
      private:
        int val;
        int mod_mask;
    };

    /* Simple queuing producer->consumer
     *
     * Each buffer is a resource that is either produced or consumed.
     * That is, a full buffer is 1 resource. Two semaphores are used
     * to track access. The 'count' values of each semaphore are used
     * to index which buffer the producer(frontend)/consumer(backend)
     * should use.
     */
    void produceEvent(const SglCompEv &ev);
    void produceEvent(const SglMemEv &ev);
    void produceEvent(const SglCFEv &ev);
    void produceEvent(const SglSyncEv &ev);
    void produceEvent(const SglCxtEv &ev);

    static int g_id;
    int m_id;
    Sem full, empty;
    Buffer *prod_buf;
    Buffer bufbuf[MAX_BUFFERS];
    CircularCounter prod_idx, cons_idx;
};


template<typename T>
inline void EventBuffer::addEvent(const T &ev)
{
    if /*not full*/(prod_buf->used < MAX_EVENTS)
    {
        produceEvent(ev);
    }
    else
    {
        empty.P();
        prod_buf = &bufbuf[prod_idx.increment()];
        full.V();
        produceEvent(ev);
    }
}


inline void EventBuffer::produceEvent(const SglMemEv &ev)
{
    assert(prod_buf != nullptr);

    auto &used = prod_buf->used;
    BufferedSglEv(&buf)[MAX_EVENTS] = prod_buf->events;

    buf[used].tag = EvTagEnum::SGL_MEM_TAG;
    buf[used].mem = ev;
    ++used;
}


inline void EventBuffer::produceEvent(const SglCompEv &ev)
{
    assert(prod_buf != nullptr);

    auto &used = prod_buf->used;
    BufferedSglEv(&buf)[MAX_EVENTS] = prod_buf->events;

    buf[used].tag = EvTagEnum::SGL_COMP_TAG;
    buf[used].comp = ev;
    ++used;
}


inline void EventBuffer::produceEvent(const SglCFEv &ev)
{
    assert(prod_buf != nullptr);

    auto &used = prod_buf->used;
    BufferedSglEv(&buf)[MAX_EVENTS] = prod_buf->events;

    buf[used].tag = EvTagEnum::SGL_CF_TAG;
    buf[used].cf = ev;
    ++used;
}


inline void EventBuffer::produceEvent(const SglSyncEv &ev)
{
    assert(prod_buf != nullptr);

    auto &used = prod_buf->used;
    BufferedSglEv(&buf)[MAX_EVENTS] = prod_buf->events;

    buf[used].tag = EvTagEnum::SGL_SYNC_TAG;
    buf[used].sync = ev;
    ++used;
}


inline void EventBuffer::produceEvent(const SglCxtEv &ev)
{
    assert(prod_buf != nullptr);

    auto &used = prod_buf->used;
    BufferedSglEv(&buf)[MAX_EVENTS] = prod_buf->events;

    buf[used].tag = EvTagEnum::SGL_CXT_TAG;
    buf[used].cxt = ev;
    ++used;
}

}; //end namespace sgl

#endif

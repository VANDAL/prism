#ifndef SGL_EVENTBUFFER_H
#define SGL_EVENTBUFFER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <cassert>

#include "Backends.hpp"

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

template<int N>
struct CircularCounter
{
    static_assert((N >= 2) && ((N & (N - 1)) == 0), "N must be a power of 2");
    CircularCounter()
    {
        val = 0;
    }
    int& operator++()
    {
        val = (val + 1) & (N-1);
        return val;
    }
    int operator++(int)
    {
        int temp = val;
        val = (val + 1) & (N-1);
        return temp;
    }
  private:
    int val;
};


namespace sgl
{

/* fudge numbers */
constexpr unsigned int MAX_EVENTS = 100000;
constexpr unsigned int MAX_BUFFERS = 8;
static_assert((MAX_BUFFERS >= 2) && ((MAX_BUFFERS & (MAX_BUFFERS - 1)) == 0),
              "Must be a power of 2");

/**
 * Simple queuing producer->consumer
 *
 * Each buffer is a resource that is either produced or consumed. That is,
 * a full buffer is one resource.  The producer(frontend) and consumer(backend)
 * each access a separate buffer at a time.
 */
class EventBuffer
{
  public:
    EventBuffer();
    ~EventBuffer();

    /* Move event to buffer used by backend */
    template<typename T>
    void addEvent(const T &ev);

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
    template <typename T>
    void produceEvent(const T &ev);

    struct Buffer
    {
        BufferedSglEv events[MAX_EVENTS];
        size_t used = 0;
    } bufbuf[MAX_BUFFERS];
    Buffer *prod_buf;

    Sem full, empty;
    CircularCounter<MAX_BUFFERS> prod_idx, cons_idx;
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
        prod_buf = &bufbuf[++prod_idx];
        full.V();

        produceEvent(ev);
    }
}


template <>
inline void EventBuffer::produceEvent(const SglMemEv &ev)
{
    prod_buf->events[prod_buf->used].tag  = EvTagEnum::SGL_MEM_TAG;
    prod_buf->events[prod_buf->used++].mem  = ev;
}


template <>
inline void EventBuffer::produceEvent(const SglCompEv &ev)
{
    prod_buf->events[prod_buf->used].tag  = EvTagEnum::SGL_COMP_TAG;
    prod_buf->events[prod_buf->used++].comp = ev;
}


template <>
inline void EventBuffer::produceEvent(const SglCFEv &ev)
{
    prod_buf->events[prod_buf->used].tag  = EvTagEnum::SGL_CF_TAG;
    prod_buf->events[prod_buf->used++].cf   = ev;
}


template <>
inline void EventBuffer::produceEvent(const SglSyncEv &ev)
{
    prod_buf->events[prod_buf->used].tag  = EvTagEnum::SGL_SYNC_TAG;
    prod_buf->events[prod_buf->used++].sync = ev;
}


template <>
inline void EventBuffer::produceEvent(const SglCxtEv &ev)
{
    prod_buf->events[prod_buf->used].tag  = EvTagEnum::SGL_CXT_TAG;
    prod_buf->events[prod_buf->used++].cxt  = ev;
}

}; //end namespace sgl

#endif

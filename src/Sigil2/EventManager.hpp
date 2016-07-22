#ifndef SGL_EVENTMANAGER_H
#define SGL_EVENTMANAGER_H

#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <mutex>

#include "Primitive.h"
#include "SigiLog.hpp"
#include "Backends.hpp"
#include "EventBuffer.hpp"

/**
 * The sigil EventManager receives sigil event primitives from
 * a frontend, typically an instrumentation tool. These events
 * are buffered, and full buffers are consumed by the backend
 * in a separate thread.
 *
 * The backend is configured during construction via a factory
 * function to spawn a backend for each thread.
 */

namespace sgl
{

class EventManager
{
    using BackendFactory = std::function<std::shared_ptr<Backend>(void)>;

  public:
    /* Create num_threads backends from the BackendFactory
     * in num_threads threads */
    EventManager(int num_threads, BackendFactory factory);
    EventManager(const EventManager &) = delete;
    EventManager &operator=(const EventManager &) = delete;
    ~EventManager();

    template<typename T>
    void addEvent(const T &ev, uint16_t buf_idx = 0);

    void finish();

  private:
    const BackendFactory backend_factory;

    /* The frontends forward events to these buffers.
     * If Sigil2 is configured in to process events with
     * a N-threaded backend, there will be N EventBuffers;
     * each EventBuffer will use the backend callbacks, so
     * those callbacks will need to be thread-safe. That
     * responsibility is on the backend developer. */
    std::vector<std::shared_ptr<EventBuffer>> frontend_buffers;

    /* plugins implemented as separate thread */
    std::vector<std::thread> consumers;
    bool finish_consumers;

    /* The backend is created and run in this
     * function for each Sigil backend thread */
    void consumeEvents(EventBuffer &buffer);
};


template<typename T>
inline void EventManager::addEvent(const T &ev, uint16_t buf_idx)
{
    assert(buf_idx < frontend_buffers.size());
    frontend_buffers[buf_idx]->addEvent(ev);
}

}; //end namespace sigil

#endif

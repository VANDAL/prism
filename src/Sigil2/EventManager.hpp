#ifndef SGL_EVENTMANAGER_H
#define SGL_EVENTMANAGER_H

#include <functional>
#include <vector>

#include "Primitive.h"

/**
 * Tracks events coming from the event generator instrumentation front-end;
 * this manager is accessed via a singleton instance.
 *
 * Event primitives are added to a buffer, which is then flushed to all 
 * registered observers. 
 *
 * finish() is expected to be called just before the end of its lifetime.
 *
 */
namespace sgl
{
constexpr unsigned int sgl_max_events = 10000;
template<typename T> using Observers = std::vector<std::function<void(T)>>; 
using CleanupObservers = std::vector<std::function<void()>>; 

struct BufferedEvent
{
	void (*notify)(const BufferedEvent&);
	void* observers;
	union 
	{
		SglMemEv  mem_ev;
		SglCompEv comp_ev;
		SglCFEv   cf_ev;
		SglCxtEv  cxt_ev;
		SglSyncEv sync_ev;
	};
};

class EventManager
{
public:
	static EventManager& instance()
	{
		static EventManager mgr;
		return mgr;
	}

	void addObserver(std::function<void(SglMemEv)> obs);
	void addObserver(std::function<void(SglCompEv)> obs);
	void addObserver(std::function<void(SglSyncEv)> obs);
	void addObserver(std::function<void(SglCxtEv)> obs);
	void addCleanup(std::function<void(void)> obs);
	void finish();
	void flushEvents();

private:
	EventManager() { used = 0; }
	EventManager(const EventManager&) = delete;
	EventManager(EventManager&&) = delete;
	EventManager& operator=(const EventManager&) = delete;
	EventManager& operator=(EventManager&&) = delete;

	void bufferEvent(SglMemEv ev);
	void bufferEvent(SglCompEv ev);
	void bufferEvent(SglSyncEv ev);
	void bufferEvent(SglCxtEv ev);
	void flushEventsIfFull();

public:
	template<typename T>
	void addEvent(T ev)
	{
		flushEventsIfFull();
		bufferEvent(ev);
	}

private:
	BufferedEvent ev_buf[sgl_max_events];
	UInt used;

	Observers<SglMemEv> mem_observers;
	Observers<SglCompEv> comp_observers;
	Observers<SglSyncEv> sync_observers;
	Observers<SglCxtEv> cxt_observers;
	CleanupObservers cleanup_observers;
};
}; //end namespace sigil

#endif

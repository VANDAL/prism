#include "EventManager.hpp"
#include <thread>
#include <chrono>
#include <cassert>

namespace sgl
{

void EventManager::startConsumer()
{
	consumer = std::thread(&EventManager::consumeEvents, this);
	consumer.detach();
}

void EventManager::consumeEvents()
{
	while (1)
	{
		full.P();
		cons_buf = &buf[cons_idx.increment()];
		if/*full*/( cons_buf->used == MAX_EVENTS )
		{
			flushEvents(*cons_buf);
			empty.V();
		}
	}
}

void EventManager::flushEvents(Buffer& buf)
{
	for (UInt i=0; i < buf.used; ++i)
	{
		buf.events[i].notifyObservers(buf.events[i]);
	}
	buf.used = 0;
}

void EventManager::finish()
{
	//let consumer finish and flush remaning buffers
	while ( full.num > 0 ) 
	{
		using namespace std::chrono;
		std::this_thread::sleep_for(milliseconds(500));
	}
	flushEvents(*prod_buf);

	//let everyone know its cleanup time
	for( auto& cleanup : cleanup_observers )
	{
		cleanup();
	}
}

void EventManager::addObserver(std::function<void(SglMemEv)> obs)
{
	mem_observers.push_back(obs);
}

void EventManager::addObserver(std::function<void(SglCompEv)> obs)
{
	comp_observers.push_back(obs);
}

void EventManager::addObserver(std::function<void(SglSyncEv)> obs)
{
	sync_observers.push_back(obs);
}

void EventManager::addObserver(std::function<void(SglCxtEv)> obs)
{
	cxt_observers.push_back(obs);
}

void EventManager::addCleanup(std::function<void()> obs)
{
	cleanup_observers.push_back(obs);
}

void EventManager::notifyMemObservers(const BufferedEvent& ev)
{
	for( auto& notify : *reinterpret_cast<Observers<SglMemEv>*>(ev.observers) )
	{
		notify(ev.mem_ev);
	}
}

void EventManager::notifyCompObservers(const BufferedEvent& ev)
{
	for( auto& notify : *reinterpret_cast<Observers<SglCompEv>*>(ev.observers) )
	{
		notify(ev.comp_ev);
	}
}

void EventManager::notifySyncObservers(const BufferedEvent& ev)
{
	for( auto& notify : *reinterpret_cast<Observers<SglSyncEv>*>(ev.observers) )
	{
		notify(ev.sync_ev);
	}
}

void EventManager::notifyCxtObservers(const BufferedEvent& ev)
{
	for( auto& notify : *reinterpret_cast<Observers<SglCxtEv>*>(ev.observers) )
	{
		notify(ev.cxt_ev);
	}
}

void EventManager::produceEvent(const SglMemEv& ev)
{
	UInt& used = prod_buf->used;
	BufferedEvent (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].notifyObservers = notifyMemObservers;
	buf[used].observers = reinterpret_cast<void*>(&mem_observers);
	buf[used].mem_ev = ev;
	used++;
}
void EventManager::produceEvent(const SglCompEv& ev)
{
	UInt& used = prod_buf->used;
	BufferedEvent (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].notifyObservers = notifyCompObservers;
	buf[used].observers = reinterpret_cast<void*>(&comp_observers);
	buf[used].comp_ev = ev;
	used++;
}
void EventManager::produceEvent(const SglSyncEv& ev)
{
	UInt& used = prod_buf->used;
	BufferedEvent (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].notifyObservers = notifySyncObservers;
	buf[used].observers = reinterpret_cast<void*>(&sync_observers);
	buf[used].sync_ev = ev;
	used++;
}
void EventManager::produceEvent(const SglCxtEv& ev)
{
	UInt& used = prod_buf->used;
	BufferedEvent (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].notifyObservers = notifyCxtObservers;
	buf[used].observers = reinterpret_cast<void*>(&cxt_observers);
	buf[used].cxt_ev = ev;
	used++;
}

}; //end namespace sgl

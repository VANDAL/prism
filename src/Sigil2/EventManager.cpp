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
	assert( prod_buf != nullptr );

	while (finish_consumer == false)
	{
		full.P();
		cons_buf = &bufbuf[cons_idx.increment()];
		if/*full*/( cons_buf->used == MAX_EVENTS )
		{
			flushNotifications(*cons_buf);
			empty.V();
		}
	}
}

void EventManager::finish()
{
	assert( prod_buf != nullptr );

	//let consumer finish and flush remaning buffers
	while ( full.count > 0 ) 
	{
		using namespace std::chrono;
		std::this_thread::sleep_for(milliseconds(500));
	}
	flushNotifications(*prod_buf);
	finish_consumer = true;

	//let everyone know its cleanup time
	for( auto& cleanup : cleanup_observers )
	{
		cleanup();
	}
}

void EventManager::flushNotifications(EventBuffer& buf)
{
	for (UInt i=0; i < buf.used; ++i)
	{
		BufferedSglEv& ev = buf.events[i];
		switch(ev.tag)
		{
		case EvTag::SGL_MEM_TAG:
			for (auto notify : mem_observers)
			{
				notify(ev.mem);
			}
			break;
		case EvTag::SGL_COMP_TAG:
			for (auto notify : comp_observers)
			{
				notify(ev.comp);
			}
			break;
		case EvTag::SGL_SYNC_TAG:
			for (auto notify : sync_observers)
			{
				notify(ev.sync);
			}
			break;
		default:
			/* Context and Control Flow events not implemented yet */
			break;
		}
	}
	buf.used = 0;
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

void EventManager::produceEvent(const SglMemEv& ev)
{
	assert( prod_buf != nullptr );

	UInt& used = prod_buf->used;
	BufferedSglEv (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].tag = EvTag::SGL_MEM_TAG;
	buf[used].mem = ev;
	++used;
}
void EventManager::produceEvent(const SglCompEv& ev)
{
	assert( prod_buf != nullptr );

	UInt& used = prod_buf->used;
	BufferedSglEv (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].tag = EvTag::SGL_COMP_TAG;
	buf[used].comp = ev;
	++used;
}
void EventManager::produceEvent(const SglSyncEv& ev)
{
	assert( prod_buf != nullptr );

	UInt& used = prod_buf->used;
	BufferedSglEv (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].tag = EvTag::SGL_SYNC_TAG;
	buf[used].sync = ev;
	++used;
}
void EventManager::produceEvent(const SglCxtEv& ev)
{
	assert( prod_buf != nullptr );

	UInt& used = prod_buf->used;
	BufferedSglEv (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].tag = EvTag::SGL_CXT_TAG;
	buf[used].cxt = ev;
	++used;
}

}; //end namespace sgl

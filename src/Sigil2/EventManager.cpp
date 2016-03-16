#include <thread>
#include <chrono>
#include <cassert>

#include "EventManager.hpp"

namespace sgl
{

EventManager::EventManager() :
	full(0),empty(MAX_BUFFERS),
	prod_idx(MAX_BUFFERS), cons_idx(MAX_BUFFERS)
{
	/* initialize producer-consumer state */
	empty.P();
	prod_buf = &bufbuf[prod_idx.increment()];
	finish_consumer = false;

	/* start consumer */
	consumer = std::thread(&EventManager::consumeEvents, this);
}


EventManager::~EventManager()
{
	/* if consumer never finished,
	 * then something bad must have interrupted
	 * normal execution. Detach consumer and warn
	 * the user */
	if (consumer.joinable() == true)
	{
		SigiLog::warn("unexpected exit: event generation did not complete");
		consumer.detach();
	}
}


void EventManager::consumeEvents()
{
	assert(prod_buf != nullptr);

	while(finish_consumer == false || empty.count < MAX_BUFFERS)
	{
		full.P();
		flushNotifications(bufbuf[cons_idx.increment()]);
		empty.V();
	}
}


void EventManager::finish()
{
	assert(prod_buf != nullptr);

	finish_consumer = true;
	full.V(); // signal that the partially full buffer can be consumed

	consumer.join();

	for(auto& cleanup : cleanup_observers)
	{
		cleanup();
	}
}


void EventManager::flushNotifications(EventBuffer &buf)
{
	for(uint32_t i=0; i<buf.used; ++i)
	{
		BufferedSglEv &ev = buf.events[i];
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
		case EvTag::SGL_CXT_TAG:
			for (auto notify : cxt_observers)
			{
				notify(ev.cxt);
			}
			break;
		default:
			/* control flow unimplemented */
			SigiLog::fatal("Received unhandled event in EventManager");
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

}; //end namespace sgl

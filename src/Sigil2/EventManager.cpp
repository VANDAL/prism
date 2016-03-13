#include <thread>
#include <chrono>
#include <cassert>

#include "EventManager.hpp"
#include "spdlog.h"

namespace sgl
{

void EventManager::consumeEvents()
{
	assert(prod_buf != nullptr);

	try
	{
		while(finish_consumer == false || empty.count < MAX_BUFFERS)
		{
			full.P();
			flushNotifications(bufbuf[cons_idx.increment()]);
			empty.V();
		}
	}
	catch(std::exception &e)
	{
		spdlog::get("sigil2-err")->info() << "error: " << e.what();
		exit(EXIT_FAILURE);
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
			throw std::runtime_error("Received unhandled event in EventManager");
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


void EventManager::produceEvent(const SglMemEv &ev)
{
	assert(prod_buf != nullptr);

	UInt& used = prod_buf->used;
	BufferedSglEv (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].tag = EvTag::SGL_MEM_TAG;
	buf[used].mem = ev;
	++used;
}


void EventManager::produceEvent(const SglCompEv &ev)
{
	assert(prod_buf != nullptr);

	UInt& used = prod_buf->used;
	BufferedSglEv (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].tag = EvTag::SGL_COMP_TAG;
	buf[used].comp = ev;
	++used;
}


void EventManager::produceEvent(const SglSyncEv &ev)
{
	assert(prod_buf != nullptr);

	UInt& used = prod_buf->used;
	BufferedSglEv (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].tag = EvTag::SGL_SYNC_TAG;
	buf[used].sync = ev;
	++used;
}


void EventManager::produceEvent(const SglCxtEv &ev)
{
	assert(prod_buf != nullptr);

	UInt& used = prod_buf->used;
	BufferedSglEv (&buf)[MAX_EVENTS] = prod_buf->events;

	buf[used].tag = EvTag::SGL_CXT_TAG;
	buf[used].cxt = ev;
	++used;
}

}; //end namespace sgl

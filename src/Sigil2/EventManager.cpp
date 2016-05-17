#include <thread>
#include <chrono>
#include <cassert>

#include "EventManager.hpp"

namespace sgl
{

EventManager::EventManager(uint32_t num_threads,
			BackendFactory factory) 
	: backend_factory(factory)
{
	assert(factory != nullptr);

	/* initialize producer-consumer state */
	finish_consumers = false;

	/* create buffers and start their consumers */
	frontend_buffers.clear();
	consumers.clear();

	for(uint32_t i=0; i<num_threads; ++i)
	{
		frontend_buffers.push_back(std::make_shared<EventBuffer>());
		consumers.push_back(std::thread(&EventManager::consumeEvents,
										this,
										std::ref(*frontend_buffers[i])));
	}
}


EventManager::~EventManager()
{
	bool warn = false;

	for(std::thread &consumer : consumers)
	{
		/* if consumer never finished,
		 * then something bad must have interrupted
		 * normal execution. Detach consumer and warn
		 * the user */
		if (consumer.joinable() == true)
		{
			warn = true;
			consumer.detach();
		}
	}

	if(warn == true)
	{
		SigiLog::warn("unexpected exit: event generation did not complete");
	}
}


void EventManager::consumeEvents(EventBuffer &buffer)
{
	auto backend = backend_factory();
	while(finish_consumers == false || buffer.isEmpty() == false)
	{
		buffer.flush(*backend);
	}
}


void EventManager::finish()
{
	finish_consumers = true;

	for(auto &buffer : frontend_buffers)
	{
		buffer->complete();
	}

	for(std::thread &consumer : consumers)
	{
		consumer.join();
	}
}

}; //end namespace sgl

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
	assert( cons_buf != nullptr && prod_buf != nullptr );

	while (finish_consumer == false)
	{
		full.P();
		cons_buf = &buf[cons_idx.increment()];
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

void EventManager::flushNotifications(NotificationBuffer& buf)
{
	for (UInt i=0; i < buf.used; ++i)
	{
		buf.notifications[i].notifyObservers(buf.notifications[i]);
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

void EventManager::notifyMemObservers(const EventNotifcation& notification)
{
	for( auto& notify : *reinterpret_cast<Observers<SglMemEv>*>(notification.observers) )
	{
		notify(notification.ev.mem);
	}
}

void EventManager::notifyCompObservers(const EventNotifcation& notification)
{
	for( auto& notify : *reinterpret_cast<Observers<SglCompEv>*>(notification.observers) )
	{
		notify(notification.ev.comp);
	}
}

void EventManager::notifySyncObservers(const EventNotifcation& notification)
{
	for( auto& notify : *reinterpret_cast<Observers<SglSyncEv>*>(notification.observers) )
	{
		notify(notification.ev.sync);
	}
}

void EventManager::notifyCxtObservers(const EventNotifcation& notification)
{
	for( auto& notify : *reinterpret_cast<Observers<SglCxtEv>*>(notification.observers) )
	{
		notify(notification.ev.cxt);
	}
}

void EventManager::produceEvent(const SglMemEv& ev)
{
	assert( prod_buf != nullptr );
	UInt& used = prod_buf->used;
	EventNotifcation (&buf)[MAX_EVENTS] = prod_buf->notifications;

	buf[used].notifyObservers = notifyMemObservers;
	buf[used].observers = reinterpret_cast<void*>(&mem_observers);
	buf[used].ev.mem = ev;
	used++;
}
void EventManager::produceEvent(const SglCompEv& ev)
{
	assert( prod_buf != nullptr );
	UInt& used = prod_buf->used;
	EventNotifcation (&buf)[MAX_EVENTS] = prod_buf->notifications;

	buf[used].notifyObservers = notifyCompObservers;
	buf[used].observers = reinterpret_cast<void*>(&comp_observers);
	buf[used].ev.comp = ev;
	used++;
}
void EventManager::produceEvent(const SglSyncEv& ev)
{
	assert( prod_buf != nullptr );
	UInt& used = prod_buf->used;
	EventNotifcation (&buf)[MAX_EVENTS] = prod_buf->notifications;

	buf[used].notifyObservers = notifySyncObservers;
	buf[used].observers = reinterpret_cast<void*>(&sync_observers);
	buf[used].ev.sync = ev;
	used++;
}
void EventManager::produceEvent(const SglCxtEv& ev)
{
	assert( prod_buf != nullptr );
	UInt& used = prod_buf->used;
	EventNotifcation (&buf)[MAX_EVENTS] = prod_buf->notifications;

	buf[used].notifyObservers = notifyCxtObservers;
	buf[used].observers = reinterpret_cast<void*>(&cxt_observers);
	buf[used].ev.cxt = ev;
	used++;
}

}; //end namespace sgl

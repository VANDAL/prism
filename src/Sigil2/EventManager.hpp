#ifndef SGL_EVENTMANAGER_H
#define SGL_EVENTMANAGER_H

#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "Primitive.h"

/**
 * The sigil EventManager receives sigil event primitives from 
 * a frontend, typically an instrumentation tool. These events
 * are buffered, and full buffers are consumed by the backend
 * in a separate thread.
 *
 * Multiple buffers exist to ensure that the backend is never
 * waiting for data to consume.
 */
namespace sgl
{

/* fudge numbers */
constexpr unsigned int MAX_EVENTS = 100000;
constexpr unsigned int MAX_BUFFERS = 15; 

template<typename T> using Observers = std::vector<std::function<void(T)>>; 
using CleanupObservers = std::vector<std::function<void()>>; 

class EventManager
{
public:
	EventManager() : 
		full(0),empty(MAX_BUFFERS),
		prod_idx(MAX_BUFFERS), cons_idx(MAX_BUFFERS)
	{ 
		empty.P();
		prod_buf = &bufbuf[prod_idx.increment()];
		finish_consumer = false;
		startConsumer();
	}
	EventManager(const EventManager&) = delete;
	EventManager& operator=(const EventManager&) = delete;

	/* Plugins add observers in the form of function callbacks */
	void addObserver(std::function<void(SglMemEv)> obs);
	void addObserver(std::function<void(SglCompEv)> obs);
	void addObserver(std::function<void(SglSyncEv)> obs);
	void addObserver(std::function<void(SglCxtEv)> obs);
	void addCleanup(std::function<void()> obs);
	void finish();

	/* Simple queuing producer->consumer
	 * 
	 * Each buffer is a resource that is either produced or consumed. 
	 * That is, a full buffer is 1 resource. Two semaphores are used
	 * to track access. The 'count' values of each semaphore are used
	 * to index which buffer the producer(frontend)/consumer(backend) 
	 * should use. 
	 * */
	//TODO clean up implementation, cohesion is lacking here...
	template<typename T>
	void addEvent(const T& ev)
	{
		if/*not full*/( prod_buf->used < MAX_EVENTS )
		{
			produceEvent( ev );
		}
		else
		{
			empty.P();
			prod_buf = &bufbuf[prod_idx.increment()];
			full.V();
			produceEvent( ev );
		}
	}

private:
	Observers<SglMemEv> mem_observers;
	Observers<SglCompEv> comp_observers;
	Observers<SglSyncEv> sync_observers;
	Observers<SglCxtEv> cxt_observers;
	CleanupObservers cleanup_observers;

	/* Simple semaphore implementation */
	class Sem 
	{
	public:
		Sem (int init) : count(count_), count_(init) {}
		int P()
		{
			std::unique_lock<std::mutex> ulock(mut);
			cond.wait(ulock, [&]{ return count > 0; });
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

		const unsigned int& count;
	private:
		unsigned int count_;
		std::mutex mut;
		std::condition_variable cond;
	};

	/* Circular counter to help indexing if more buffers
	 * are decided in the future. */
	struct CircularCounter
	{
		CircularCounter( int mod_val ) : mod_val(mod_val) {val = 0;}
		int increment() 
		{
			int old_val = val;
			if (++val == mod_val)
			{
				val = 0;
			}
			return old_val;
		}
	private:
		int val;
		int mod_val;
	};

	struct EventBuffer 
	{
		BufferedSglEv events[MAX_EVENTS];
		UInt used = 0;
	};

	Sem full, empty;
	EventBuffer *prod_buf, *cons_buf;
	EventBuffer bufbuf[MAX_BUFFERS];
	CircularCounter prod_idx, cons_idx;
	void produceEvent(const SglMemEv& ev);
	void produceEvent(const SglCompEv& ev);
	void produceEvent(const SglSyncEv& ev);
	void produceEvent(const SglCxtEv& ev);

	/* plugins implemented as separate thread */
	std::thread consumer;
	void startConsumer();
	bool finish_consumer;
	void consumeEvents();
	void flushNotifications(EventBuffer& buf);
};
}; //end namespace sigil

#endif

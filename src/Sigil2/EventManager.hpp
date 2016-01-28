#ifndef SGL_EVENTMANAGER_H
#define SGL_EVENTMANAGER_H

#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <iostream>

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

/* fudge numbers */
constexpr unsigned int MAX_EVENTS = 100000;
constexpr unsigned int MAX_BUFFERS = 15; 

template<typename T> using Observers = std::vector<std::function<void(T)>>; 
using CleanupObservers = std::vector<std::function<void()>>; 

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
	void addCleanup(std::function<void()> obs);
	void finish();

	/* Simple queuing producer->consumer
	 * 
	 * Each buffer is a resource that is either produced or consumed. 
	 * That is, a full buffer = 1 resource. Two semaphores are used
	 * to track access. The 'count' values of each semaphore are used
	 * to index which buffer the producer(frontend)/consumer(backend) 
	 * should use. 
	 * */
	//TODO clean up implementation, cohesion is lacking here...
	template<typename T>
	void addEvent(T ev)
	{
		if/*not full*/( prod_buf->used < MAX_EVENTS )
		{
			produceEvent( ev );
		}
		else
		{
			empty.P();
			prod_buf = &buf[prod_idx.increment()];
			full.V();
			produceEvent( ev );
		}
	}

private:
	EventManager() : 
		full(0),empty(MAX_BUFFERS),
		prod_idx(MAX_BUFFERS), cons_idx(MAX_BUFFERS)
	{ 
		empty.P();
		prod_buf = &buf[prod_idx.increment()];
		startConsumer();
	}
	EventManager(const EventManager&) = delete;
	EventManager(EventManager&&) = delete;
	EventManager& operator=(const EventManager&) = delete;
	EventManager& operator=(EventManager&&) = delete;

	Observers<SglMemEv> mem_observers;
	Observers<SglCompEv> comp_observers;
	Observers<SglSyncEv> sync_observers;
	Observers<SglCxtEv> cxt_observers;
	CleanupObservers cleanup_observers;

	/* One structure to hold any type of Sigil event.
	 * Buffering events in an constant sized array
	 * turns out to be much faster than allocating each
	 * new event on the heap, for billions+ events 
	 *
	 * Each BufferedEvent has 
	 *	- a list of all observers to be called
	 *	- the event itself
	 *	- a pointer to a function that knows the correct event type, 
	 *	  so it can correctly notify all observers
	 *
	 *    ML: I felt this function call would be faster 
	 *    than looking at a type tag, for every event
	 * */
	struct BufferedEvent
	{
		void (*notifyObservers)(const BufferedEvent&);
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
	static void notifyMemObservers( const BufferedEvent& ev );
	static void notifyCompObservers( const BufferedEvent& ev );
	static void notifySyncObservers( const BufferedEvent& ev );
	static void notifyCxtObservers( const BufferedEvent& ev );

	struct Buffer 
	{
		BufferedEvent events[MAX_EVENTS];
		UInt used = 0;
	};

	class Sem 
	{
	public:
		Sem (int init) : num(num_), num_(init) {}
		int P()
		{
			std::unique_lock<std::mutex> ulock(mut);
			cond.wait(ulock, [&]{ return num > 0; });
			--num_;
			return num_;
		}
		int V()
		{
			std::unique_lock<std::mutex> ulock(mut);
			++num_;
			cond.notify_one();
			return num_;
		}

		const int& num;
	private:
		int num_;
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

	Sem full, empty;
	Buffer *prod_buf, *cons_buf;
	Buffer buf[MAX_BUFFERS];
	CircularCounter prod_idx, cons_idx;
	void produceEvent(const SglMemEv& ev);
	void produceEvent(const SglCompEv& ev);
	void produceEvent(const SglSyncEv& ev);
	void produceEvent(const SglCxtEv& ev);

	std::thread consumer;
	void startConsumer();
	void consumeEvents();
	void flushEvents(Buffer& buf);
};
}; //end namespace sigil

#endif

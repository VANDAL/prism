#ifndef SGL_EVENTMANAGER_H
#define SGL_EVENTMANAGER_H

#include <functional>
#include <vector>

#include "SglInit.hpp"
#include "Primitive.h"

/*
 * Tracks events coming from the front-end event generator
 *
 * The front-end queues up events for processing.
 */
namespace sgl
{
	constexpr unsigned int sgl_max_events = 10000;
	template<typename T> using Observers = std::vector<std::function<void(T)>>; 

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
			EventManager();
			void flushEvents();
			void addObserver(std::function<void(SglMemEv)> obs);
			void addObserver(std::function<void(SglCompEv)> obs);
			void addObserver(std::function<void(SglSyncEv)> obs);
			void addObserver(std::function<void(SglCxtEv)> obs);

		private:
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
	};
};

#endif

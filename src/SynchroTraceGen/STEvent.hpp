#ifndef STGEN_EVENT_H
#define STGEN_EVENT_H

#include <vector>
#include <unordered_map>
#include "Sigil2/Primitive.h"

/*******************************************************************************
 * SynchroTrace Events
 *
 * Defines an application event trace for later replay
 * in the SynchroTraceSim CMP simulator.
 *
 * Please see Section 2.B in the SynchroTrace paper for further clarification.
 ******************************************************************************/

namespace STGen
{
	using EId = long long;
	using TId = int;

	/* Helper class to track unique ranges of addresses
	 * Unordered */
	struct AddrRange
	{
		std::vector<std::pair<Addr,Addr>> ranges;

		public:
			bool insert(Addr begin, Addr end);
			void clear();

		private:
			/* Returns true if addr range 2 contains addr range 1, inclusive
			 * Returns false otherwise */
			bool addrOverlap(
					Addr addr1_begin, Addr addr1_end,
					Addr addr2_begin, Addr addr2_end
					);
	};

	/**
	 * Base SynchroTrace event. SynchroTrace events can be made of multiple
	 * Sigil event primitives.
	 *
	 * All SynchroTrace Events have the thread id that generated the event,
	 * and an event id within that thread.
	 */
	struct STEvent
	{
		/**
		 * Per-thread event count. Logged to SynchroTrace event trace.
		 * Each derived SynchroTrace event tracks the same event id.
		 */
		//TODO make thread/event tracking thread-safe
		static std::unordered_map<TId, EId> event_ids;
		static TId curr_thread_id;
		static EId curr_event_id;
		static void setThread(TId tid);

		public:
			/// Flush an event to storage and increment event counter
			virtual void flush() final; 

		private: 
			virtual void detailedFlush() = 0;
			virtual void reset() = 0;

		protected:
			bool is_active = false;
	};

	/**
	 * A SynchroTrace Compute Event.
	 *
	 * SynchroTrace compute events comprise of one or more Sigil primitive events.
	 * I.e., a ST compute event can be any number of local thread load, store,
	 * and compute primitives. However in practice, a 'compute' event is normally 
	 * cut off at ~100 event primitives.
	 * 
	 * A SynchroTrace compute event is only valid for a given thread. 
	 * Usage is to fill up the event, then flush it to storage at a thread swap,
	 * a communication edge between threads, or at an arbitrary number 
	 * of iops/flops/reads/writes.
	 */
	struct STCompEvent : public STEvent
	{
		UInt iop_cnt;
		UInt flop_cnt;	
		/// Stores and Loads originating from the current thread
		UInt store_cnt;
		UInt load_cnt;
		AddrRange stores_unique; 
		AddrRange loads_unique;
		UInt total_events;

		public:
			STCompEvent();
			bool updateWrites(SglMemEv ev);
			bool updateWrites(Addr begin, Addr size);
			bool updateReads(SglMemEv ev);
			bool updateReads(Addr begin, Addr size);
			void incIOP();
			void incFLOP();
			
		private:
			virtual void detailedFlush() override;
			virtual void reset() override;
	};

	/**
	 * A SynchroTrace Communication Event.
	 *
	 * SynchroTrace communication events comprise of communication edges.
	 * I.e., a ST comm event is typically generated from a read to data
	 * that was originally created by other threads. Any subsequent reads
	 * to that data by the same thread is not considered a communication 
	 * edge. Another thread that writes to the same address resets this process.
	 */
	struct STCommEvent : public STEvent
	{
		typedef std::vector<std::tuple<TId, EId, Addr, Addr>> LoadEdges;
		/**< vector of: 
		 *     producer thread id, 
		 *     producer event id, 
		 *     addr range
		 * for reads to data written by another thread
		 */

		LoadEdges comms;	

		public:
			STCommEvent();
			/** 
			 * Adds communication edges originated from a single load/read primitive.
			 * Use this function when reading data that was written by a different thread.
			 *
			 * Expected to by called byte-by-byte. 
			 * That is, successive calls to this function imply a continuity 
			 * between addresses. If the first call specifies address 0x0000,
			 * and the next call specifies address 0x0008, then it implies
			 * addresses 0x0000-0x0008 were all read, instead of non-consecutively read.
			 *
			 * Use STEvent::flush() between different read primitives.
			 */
			void addEdge(TId writer, EId writer_event, Addr addr);

		private:
			virtual void detailedFlush() override;
			virtual void reset() override;
	};

	/**
	 * A SynchroTrace Synchronization Event.
	 *
	 * Synchronization events mark thread spawns, joins, barriers, locks, et al.
	 * Expected use is to flush immediately; these events do not aggregate like 
	 * SynchroTrace Compute or Communication events.
	 *
	 * Synchronizatin events are expected to be immediately logged.
	 */
	class STSyncEvent : public STEvent
	{
		UChar type = 0;
		Addr sync_addr = 0;

		public:
			/* Only logging is required */
			//TODO clarify name
			void logSync(UChar type, Addr sync_addr);

		private:
			virtual void detailedFlush() override;
			virtual void reset() override;
	};
};

#endif

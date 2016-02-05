#ifndef STGEN_EVENT_H
#define STGEN_EVENT_H

#include <vector>
#include <set>
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
constexpr const char filename[32] = "sigil.events-";

/* TODO add asserts that EIds and TIds 
 * aren't negative during flushing */
using EId = long long;
using TId = int;

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
	//TODO refactor without static variables
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

public:
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
	/* Helper class to track unique ranges of addresses */
	struct AddrSet
	{
		using AddrRange = std::pair<Addr,Addr>;
		/* A range of addresses is specified by the pair.
		 * This call inserts that range and merges existing ranges
		 * in order to keep the set of addresses unique */
		void insert(const AddrRange &range);
		void clear();
		const std::multiset<AddrRange>& get(){ return ms; }
	
	private:
		std::multiset<AddrRange> ms;
	};

	UInt iop_cnt;
	UInt flop_cnt;	

	/* Stores and Loads originating from the current thread
	 * I.e. non-edge mem events. These are the count for 'events',
	 * not a count for bytes stored/loaded */
	UInt store_cnt;
	UInt load_cnt;

	/* Holds a range for the addresses touched by local stores/loads */
	AddrSet stores_unique; 
	AddrSet loads_unique;

	UInt total_events;

public:
	STCompEvent();
	void updateWrites(SglMemEv ev);
	void updateWrites(Addr begin, Addr size);
	void updateReads(SglMemEv ev);
	void updateReads(Addr begin, Addr size);
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
	using STEvent::flush;

	/* extend flush from base; 
	 * only behavior is immediate flush */
	void flush(UChar type, Addr sync_addr);

private:
	virtual void detailedFlush() override;
	virtual void reset() override;
};
}; //end namespace STGen

#endif

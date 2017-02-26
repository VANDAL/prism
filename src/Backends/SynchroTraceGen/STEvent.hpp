#ifndef STGEN_EVENT_H
#define STGEN_EVENT_H

#include <memory>
#include <vector>
#include <sstream>
#include <set>
#include <unordered_map>

#include "MemoryPool.h"
#include "spdlog/spdlog.h"
#include "STShadowMemory.hpp"

/*******************************************************************************
 * SynchroTrace Events
 *
 * Defines an application event trace for later replay
 * in the SynchroTraceSim CMP simulator.
 *
 * See Section 2.B in the SynchroTrace paper for further clarification.
 ******************************************************************************/

namespace STGen
{
using StatCounter = unsigned long long;


/* For speed, adapted from http://stackoverflow.com/a/33447587
 * XXX hexStr MUST be at least (sizeof(I) * 2 + 3)
 * Returns the beginning location of the hex string, 
 * since the supplied char* will be truncated:
 *     8BADF00D0xDEADBEEF\0
 *             |----- the hex string starts here at 0x */
template <typename I>
inline const char *n2hexstr(char *hexStr, const I &w)
{
    static constexpr int hexLen = sizeof(I) * 2;
    static const char *digits = "0123456789abcdef";

    bool hexMSBfound = false;
    int hexMSB = 2;

    for (size_t i=2, j=(hexLen-1) * 4; i<hexLen+2; ++i, j-=4)
    {
        hexStr[i] = digits[(w >> j) & 0x0f];

        if (hexMSBfound == false && hexStr[i] != '0')
        {
            hexMSBfound = true;
            hexMSB = i;
        }
    }

    /* TODO if 'w' is 0, then this returns the full size hex string;
     * instead it should just return "0x0" */

    /* Truncate the string if it doesn't need the full width
     * This will save space in the ASCII formatted trace */
    hexStr[hexMSB - 2] = '0';
    hexStr[hexMSB - 1] = 'x';
    hexStr[hexLen + 2] = '\0';
    return hexStr + (hexMSB-2);
}

/* Helper class to track unique ranges of addresses */
struct AddrSet
{
    using AddrRange = std::pair<Addr, Addr>;
    AddrSet() { }
    AddrSet(const AddrRange &range)
    {
        ms.insert(range);
    }
    AddrSet(const AddrSet &other)
    {
        ms = other.ms;
    }
    AddrSet &operator=(const AddrSet &) = delete;

  private:
    std::multiset<AddrRange, std::less<AddrRange>, MemoryPool<AddrRange>> ms;

  public:
    /* A range of addresses is specified by the pair.
     * This call inserts that range and merges existing ranges
     * in order to keep the set of addresses unique */
    void insert(const AddrRange &range);
    void clear();
    const decltype(ms) &get()
    {
        return ms;
    }
};


/**
 * XXX MDL20161104
 * Instruction address not currenlty utilized
 *
 * A SynchroTrace Instruction Event
 *
 * SynchroTrace tracks instruction addresses for future work to increase
 * simulation accuracy. The addresses can be used to simulate an I-cache
 * and any associated latencies
 *
 * The addresses are aggregated to avoid needless writes
 */
struct STInstrEvent
{
    /* all active addresses */
    std::string instrs;

    bool is_empty;
    const std::shared_ptr<spdlog::logger> &logger;

    STInstrEvent(const std::shared_ptr<spdlog::logger> &logger);
    void append_instr(Addr addr);

    /* Addresses should be flushed whenever
     * compute or communication events flush */
    void flush();

  private:
    void reset();
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
struct STCompEvent
{
    StatCounter iop_cnt;
    StatCounter flop_cnt;

    /* Stores and Loads originating from the current thread
     * I.e. non-edge mem events. These are the count for 'events',
     * not a count for bytes stored/loaded */
    StatCounter thread_local_store_cnt;
    StatCounter thread_local_load_cnt;

    /* Holds a range for the addresses touched by local stores/loads */
    AddrSet stores_unique;
    AddrSet loads_unique;

    StatCounter total_events;
    bool is_empty;

    TID &thread_id;
    EID &event_id;
    std::string logmsg;
    const std::shared_ptr<spdlog::logger> &logger;

    STCompEvent(TID &tid, EID &eid, const std::shared_ptr<spdlog::logger> &logger);
    void flush();
    void updateWrites(const SglMemEv &ev);
    void updateWrites(const Addr begin, const Addr size);
    void updateReads(const SglMemEv &ev);
    void updateReads(const Addr begin, const Addr size);

    /* Compute Event metadata */
    void incWrites();
    void incReads();
    void incIOP();
    void incFLOP();

    /* HACK Hotfix requested by KS for IOP/FLOP counts on a per-thread basis,
     * and additionally on a per-process basis */
    void track_thread(TID tid);

  private:
    void reset();
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
struct STCommEvent
{
    using LoadEdges = std::vector<std::tuple<TID, EID, AddrSet>>;
    /**< vector of:
     *     producer thread id,
     *     producer event id,
     *     addr range
     * for reads to data written by another thread
     */

    LoadEdges comms;
    bool is_empty;

    TID &thread_id;
    EID &event_id;
    std::string logmsg;
    const std::shared_ptr<spdlog::logger> &logger;

    STCommEvent(TID &tid, EID &eid, const std::shared_ptr<spdlog::logger> &logger);
    void flush();

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
    void addEdge(const TID writer, const EID writer_event, const Addr addr);

  private:
    void reset();
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
using STSyncType = unsigned char;
struct STSyncEvent
{
    STSyncType type = 0;
    Addr sync_addr = 0;

    TID &thread_id;
    EID &event_id;
    std::string logmsg;
    const std::shared_ptr<spdlog::logger> &logger;

    STSyncEvent(TID &tid, EID &eid, const std::shared_ptr<spdlog::logger> &logger);

    /* only behavior is immediate flush */
    void flush(const STSyncType type, const Addr sync_addr);
};

}; //end namespace STGen

#endif

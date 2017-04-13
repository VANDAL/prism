#ifndef STGEN_EVENT_H
#define STGEN_EVENT_H

#include "AddrSet.hpp"
#include "STTypes.hpp" // Addr, TID, EID
#include "STEventTraceUncompressed.capnp.h"
#include "spdlog/spdlog.h"
#include <vector>

/******************************************************************************
 * SynchroTrace Events
 *
 * Defines an application event trace for later replay
 * in the SynchroTraceSim CMP simulator.
 *
 * See Section 2.B in the SynchroTrace paper for further clarification.
 *
 * TODO(someday) MDL20170321 Clean up uncompressed event interface
 *****************************************************************************/


namespace STGen
{

/**
 * A SynchroTrace Compute Event.
 *
 * SynchroTrace compute events comprise of one or more Sigil primitive events.
 * I.e., a ST compute event can be any number of local thread load, store,
 * and compute primitives.
 *
 * A SynchroTrace compute event is only valid for a given thread.
 * Usage is to fill up the event, then flush it to storage at a
 * thread swap, a communication edge between threads, or at an
 * arbitrary number of iops/flops/reads/writes.
 */

struct STCompEventCompressed
{
    auto updateWrites(Addr begin, Addr size) -> void;
    auto updateReads(Addr begin, Addr size) -> void;
    auto incWrites() -> void;
    auto incReads() -> void;
    auto incIOP() -> void;
    auto incFLOP() -> void;
    auto reset() -> void;

    StatCounter iops{0};
    StatCounter flops{0};

    /* Stores and Loads originating from the current thread
     * i.e. non-edge mem events.
     * These are 'event' counts, not byte counts */
    StatCounter writes{0};
    StatCounter reads{0};

    /* Holds a range for the addresses touched by local stores/loads */
    AddrSet uniqueWriteAddrs;
    AddrSet uniqueReadAddrs;

    bool isActive{false};
};

struct STCompEventUncompressed
{
    auto incIOP() -> void;
    auto incFLOP() -> void;
    auto reset() -> void;

    StatCounter iops{0};
    StatCounter flops{0};

    /* one read or write per event,
     * and the address range */
    using MemType = EventStreamUncompressed::Event::MemType;

    bool isActive{false};
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

struct STCommEventCompressed
{
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
    auto addEdge(TID writer, EID writer_event, Addr addr) -> void;
    auto reset() -> void;

    /**
     * vector of:
     * - producer thread id,
     * - producer event id,
     * - addr range
     * for reads to data written by another thread
     */
    using LoadEdges = std::vector<std::tuple<TID, EID, AddrSet>>;
    LoadEdges comms;

    bool isActive{false};
};

}; //end namespace STGen

#endif

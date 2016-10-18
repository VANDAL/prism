#ifndef SIGILCLASSIC_H
#define SIGILCLASSIC_H

#include <unordered_map>
#include <stack>
#include <string>
#include <cstdint>

#include "ShadowMemory.hpp"
#include "Sigil2/Primitive.h"

namespace SigilClassic
{

/* An entity can be any grouping of events, e.g. basic blocks, functions, or
 * threads.
 *
 * In the case of functions, SigilClassic assumes that a new 'Enter Function' is
 * an actual function call, and not a backdoor jump from another function into
 * the middle of some other function.
 */

/* Entity/thread unique ID */
using UInt = uint32_t;
using Int = int32_t;

using EID = Int;
using TID = Int;

constexpr TID INVL_TID{-1};
constexpr EID INVL_EID{-1};


/* Keeps track of entity metadata */
struct EntityData
{
    /* The same function name may be called many times.
     * Save some space by pointing to the name */
    const std::string *name{nullptr};

    /* Unique communication between entities */
    std::unordered_map<EID, UInt> comm_edges;

    /* Bytes read, that are written by this same entity */
    UInt local_bytes_read{0};

    /* IOPs/FLOPs computed in this entity */
    UInt iops{0};
    UInt flops{0};

    /* Track the call tree */
    EID caller{INVL_EID};
};


/* Keeps track of state between thread context switches */
struct TContext
{
    std::unordered_multimap<std::string, EID> entity_ids;
    std::unordered_map<EID, EntityData> entity_data;
    std::stack<EID> callstack;
    EID cur_eid{INVL_EID};
};


struct SigilContext
{
    SigilContext();

    /* Reset all the contexts to that of 'tid'.
     * Necessary because an entity (e.g. function) can be
     * interrupted before exiting when threads are switched. */
    auto setThreadContext(TID tid) -> void;

    /* Beginning or end marker of a entity.
     * Creates or destroys new metadata for the entity */
    auto enterEntity(std::string name) -> void;
    auto exitEntity() -> void;

    auto monitorWrite(Addr addr, ByteCount bytes) -> void;
    auto monitorRead(Addr addr, ByteCount bytes) -> void;
    auto incrIOPCost() -> void;
    auto incrFLOPCost() -> void;


    ShadowMemory sm;
    std::unordered_map<TID, TContext> thread_contexts;

    TID cur_tid{INVL_TID};
    EID global_eid_cnt{INVL_EID};
    TContext *cur_tcxt;

    /* tcontext cache */
    decltype(TContext::cur_eid)     *cur_eid{nullptr};
    decltype(TContext::entity_ids)  *cur_entity_ids{nullptr};
    decltype(TContext::entity_data) *cur_entity_data{nullptr};
    decltype(TContext::callstack)   *cur_callstack{nullptr};
    EntityData *cur_entity{nullptr};
};


}; //end namespace SigilClassic

#endif

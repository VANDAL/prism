#ifndef STGEN_TYPES_H
#define STGEN_TYPES_H

#include "ShadowMemory.hpp" //Addr
#include <set>

namespace STGen
{

/* XXX Thread ID (TID) and Event ID (EID)
 * set to 16-bits and 32-bits for memory usage considerations.
 * Increasing the sizes may be required in the future. */
using TID = int16_t;
using EID = uint32_t;

using StatCounter = unsigned long long;
using Stats = std::tuple<StatCounter, StatCounter, StatCounter, StatCounter, StatCounter>;
enum StatsType {IOP=0, FLOP, READ, WRITE, INSTR};
using ThreadStatMap = std::map<TID, Stats>;

/** Synchronization **/

/* Vector of:
 * - spawner
 * - address of spawnee thread_t */
using SpawnList = std::vector<std::pair<TID, Addr>>;

/* Each thread's ID in the order it was first seen */
using ThreadList = std::vector<TID>;

/* Vector of:
 * - barrier_t addr
 * - participating threads
 * Order is important */
using BarrierList = std::vector<std::pair<Addr, std::set<TID>>>;

};

#endif

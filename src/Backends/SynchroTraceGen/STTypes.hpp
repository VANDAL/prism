#ifndef STGEN_TYPES_H
#define STGEN_TYPES_H

#include "ShadowMemory.hpp" //Addr
#include "STStats.hpp"
#include <set>
#include <list>

namespace STGen
{

/* XXX Thread ID (TID) and Event ID (EID)
 * set to 16-bits and 32-bits for memory usage considerations.
 * Increasing the sizes may be required in the future. */
using TID = int16_t;
using EID = uint32_t;

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

/* Metrics per thread */
using ThreadStatMap = std::map<TID, PerThreadStats>;
};

#endif

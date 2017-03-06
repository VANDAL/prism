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

/* XXX added for SynchroLearn™ */
struct BarrierStats
{
    StatCounter iops{0};
    StatCounter flops{0};
    StatCounter instrs{0};
    StatCounter memAccesses{0};
    StatCounter locks{0};
    auto iopsPerMemAccess() -> float { return static_cast<float>(iops)/memAccesses; }
    auto flopsPerMemAccess() -> float { return static_cast<float>(flops)/memAccesses; }
    auto locksPerIopsPlusFlops() -> float { return static_cast<float>(locks)/(iops + flops); }
};

using AllBarriersStats = std::vector<std::pair<Addr, BarrierStats>>;
class PerThreadBarrierStats
{
  public:
    auto incIOPs() -> void { ++current.iops; }
    auto incFLOPs() -> void { ++current.flops; }
    auto incInstrs() -> void { ++current.instrs; }
    auto incMemAccesses() -> void { ++current.memAccesses; }
    auto incLocks() -> void { ++current.locks; }
    auto barrier(Addr id) -> void
    {
        barriers.push_back(std::make_pair(id, current));
        current = BarrierStats{};
    }

    auto getAllBarriersStats() const -> AllBarriersStats { return barriers; }

  private:
    AllBarriersStats barriers;
    BarrierStats current;
};

using ThreadStatMap = std::map<TID, std::pair<Stats, AllBarriersStats>>;

};

#endif

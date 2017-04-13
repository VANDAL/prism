#ifndef STGEN_STATS_H
#define STGEN_STATS_H

#include "ShadowMemory.hpp" //Addr
#include <tuple>
#include <list>

/* TODO(someday) these names are confusing; change them */

namespace STGen
{

using StatCounter = unsigned long long;
using Stats = std::tuple<StatCounter, StatCounter, StatCounter, StatCounter, StatCounter>;
enum StatsType {IOP=0, FLOP, READ, WRITE, INSTR};

struct BarrierStats
{
    /* XXX added for SynchroLearn™ */
    /* Statistics per barrier region */

    StatCounter iops{0};
    StatCounter flops{0};
    StatCounter instrs{0};
    StatCounter communication{0};
    StatCounter memAccesses{0};
    StatCounter locks{0};
    auto iopsPerMemAccess() -> float { return static_cast<float>(iops)/memAccesses; }
    auto flopsPerMemAccess() -> float { return static_cast<float>(flops)/memAccesses; }
    auto locksPerIopsPlusFlops() -> float { return static_cast<float>(locks)/(iops + flops); }

    BarrierStats& operator+=(const BarrierStats &rhs)
    {
        this->iops += rhs.iops;
        this->flops += rhs.flops;
        this->instrs += rhs.instrs;
        this->communication += rhs.communication;
        this->memAccesses += rhs.memAccesses;
        this->locks += rhs.locks;
        return *this;
    }

    BarrierStats operator+(const BarrierStats &rhs)
    {
        BarrierStats ret{*this};
        ret += rhs;
        return ret;
    }
};

struct LockStats
{
    /* Statistics between a lock/unlock */

    StatCounter iops{0};
    StatCounter flops{0};
    StatCounter instrs{0};
    StatCounter memAccesses{0};
    StatCounter communication{0};
};

using AllBarriersStats = std::list<std::pair<Addr, BarrierStats>>;
class PerBarrierStats
{
  public:
    auto incIOPs() -> void { ++current.iops; }
    auto incFLOPs() -> void { ++current.flops; }
    auto incInstrs() -> void { ++current.instrs; }
    auto incMemAccesses() -> void { ++current.memAccesses; }
    auto incComm() -> void { ++current.communication; }
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

using AllLocksStats = std::list<std::pair<Addr, LockStats>>;
class PerLockStats
{
    /* XXX Assumes common case of only one lock held at a time */
  public:
    auto incIOPs() -> void { if (active == true) ++current.iops; }
    auto incFLOPs() -> void { if (active == true) ++current.flops; }
    auto incInstrs() -> void { if (active == true) ++current.instrs; }
    auto incMemAccesses() -> void { if (active == true) ++current.memAccesses; }
    auto incComm() -> void { if (active == true) ++current.communication; }
    auto lock() -> void { active = true; }
    auto unlock(Addr id) -> void
    {
        locks.push_back(std::make_pair(id, current));
        current = LockStats{};
        active = false;
    }
    auto getAllLocksStats() const -> AllLocksStats { return locks; }

  private:
    AllLocksStats locks;
    LockStats current;
    bool active{false};
};

class PerThreadStats
{
  public:
    auto incIOPs() -> void
    {
        ++std::get<IOP>(stats);
        barrierStats.incIOPs();
        lockStats.incIOPs();
    }

    auto incFLOPs() -> void
    {
        ++std::get<FLOP>(stats);
        barrierStats.incFLOPs();
        lockStats.incFLOPs();
    }

    auto incInstrs() -> void
    {
        ++std::get<INSTR>(stats);
        barrierStats.incInstrs();
        lockStats.incInstrs();
    }

    auto getTotalInstrs() -> StatCounter
    {
        return std::get<INSTR>(stats);
    }

    auto incReads() -> void
    {
        ++std::get<READ>(stats);
        barrierStats.incMemAccesses();
        lockStats.incMemAccesses();
    }

    auto incWrites() -> void
    {
        ++std::get<WRITE>(stats);
        barrierStats.incMemAccesses();
        lockStats.incMemAccesses();
    }

    auto incComm() -> void
    {
        barrierStats.incComm();
        lockStats.incComm();
    }

    auto incSyncs(unsigned char type, Addr id) -> void
    {
        /* #define P_MUTEX_LK              1 */
        /* #define P_MUTEX_ULK             2 */
        /* ...                               */
        /* #define P_BARRIER_WT            5 */
        if (type == 1)
        {
            barrierStats.incLocks();
            lockStats.lock();
        }
        else if (type == 2)
        {
            lockStats.unlock(id);
        }
        else if (type == 5)
        {
            barrierStats.barrier(id);
        }
    }

    auto getTotalStats() -> Stats
    {
        return stats;
    }

    auto getBarrierStats() -> AllBarriersStats
    {
        return barrierStats.getAllBarriersStats();
    }

    auto getLockStats() -> AllLocksStats
    {
        return lockStats.getAllLocksStats();
    }

  private:
    Stats stats{0,0,0,0,0};
    PerBarrierStats barrierStats;
    PerLockStats lockStats;
};

}; //end namespace STGen

#endif

#include "TextLogger.hpp"
#include "spdlog/fmt/fmt.h"

namespace STGen
{

namespace
{

auto flushSyncEvent(unsigned char syncType, unsigned numArgs, Addr *syncArgs,
                    EID eid, TID tid,
                    std::shared_ptr<spdlog::logger> &logger) -> void
{
    assert(numArgs > 0);

    std::string logMsg = fmt::format("{:d},{:d},pth_ty:{:d}^{:#x}",
                                     eid,
                                     tid,
                                     syncType,
                                     syncArgs[0]);
    for (unsigned i = 1; i < numArgs; ++i)
        fmt::format_to(std::back_inserter(logMsg), "&{:#x}", syncArgs[i]);

    logger->info(logMsg);
}


auto flushInstrMarker(int limit, std::shared_ptr<spdlog::logger> &logger) -> void
{
    logger->info("! {}", limit);
}

}; //end namespace


TextLoggerCompressed::TextLoggerCompressed(TID tid, const std::string& outputPath)
{
    assert(tid >= 1);

    std::string filePath = fmt::format("{}/sigil.events.out-{}.gz", outputPath, tid);
    std::tie(logger, gzfile) = prism::getFileLogger<spdlog::async_factory, gzofstream>(std::move(filePath));
}


TextLoggerCompressed::~TextLoggerCompressed()
{
    prism::blockingFlushAndDeleteLogger(logger);
    /* gzofstream destructor closes gzfile  */
}


auto TextLoggerCompressed::flush(const STCompEventCompressed& ev, EID eid, TID tid) -> void
{
    fmt::format_to(std::back_inserter(logMsg),
                   "{},{},{},{},{},{}",
                   eid,
                   tid,
                   ev.iops,
                   ev.flops,
                   ev.reads,
                   ev.writes);

    for (auto &p : ev.uniqueWriteAddrs.get())
    {
        assert(p.first <= p.second);
        fmt::format_to(std::back_inserter(logMsg), " $ {:#x} {:#x}", p.first, p.second);
    }

    for (auto &p : ev.uniqueReadAddrs.get())
    {
        assert(p.first <= p.second);
        fmt::format_to(std::back_inserter(logMsg), " * {:#x} {:#x}", p.first, p.second);
    }

    logger->info(logMsg);
    logMsg.clear();
}


auto TextLoggerCompressed::flush(const STCommEventCompressed& ev, EID eid, TID tid) -> void
{
    assert(ev.comms.empty() == false);

    fmt::format_to(std::back_inserter(logMsg), "{},{}", eid, tid);
    for (auto &edge : ev.comms)
        for (auto &p : std::get<2>(edge).get())
            fmt::format_to(std::back_inserter(logMsg), " # {} {} {:#x} {:#x}",
                           std::get<0>(edge), std::get<1>(edge), p.first, p.second);

    logger->info(logMsg);
    logMsg.clear();
}


auto TextLoggerCompressed::flush(unsigned char syncType, unsigned numArgs, Addr *syncArgs,
                                 EID eid, TID tid) -> void
{
    flushSyncEvent(syncType, numArgs, syncArgs, eid, tid, logger);
}


auto TextLoggerCompressed::instrMarker(int limit) -> void
{
    flushInstrMarker(limit, logger);
}


TextLoggerUncompressed::TextLoggerUncompressed(TID tid, const std::string& outputPath)
{
    assert(tid >= 1);

    std::string filePath = fmt::format("{}/sigil.events.out-{}.gz", outputPath, tid);
    std::tie(logger, gzfile) = prism::getFileLogger<spdlog::async_factory, gzofstream>(std::move(filePath));
}


TextLoggerUncompressed::~TextLoggerUncompressed()
{
    prism::blockingFlushAndDeleteLogger(logger);
    /* gzofstream destructor closes gzfile  */
}


auto TextLoggerUncompressed::flush(StatCounter iops, StatCounter flops,
                                   STCompEventUncompressed::MemType type, Addr start, Addr end,
                                   EID eid, TID tid) -> void
{
    fmt::format_to(std::back_inserter(logMsg), "{},{},{},{}", eid, tid, iops, flops);

    switch (type)
    {
    /* only one of
     *  - no reads/writes,
     *  - one read,
     *  - one write
     * possible in uncompressed mode */
    case STCompEventUncompressed::MemType::READ:
        fmt::format_to(std::back_inserter(logMsg), ",1,0 * {:#x} {:#x}", start, end);
        break;
    case STCompEventUncompressed::MemType::WRITE:
        fmt::format_to(std::back_inserter(logMsg), ",0,1 $ {:#x} {:#x}", start, end);
        break;
    case STCompEventUncompressed::MemType::NONE:
        logMsg += ",0,0";
        break;
    default:
        fatal("textlogger encountered unhandled memory type");
    }

    logger->info(logMsg);
    logMsg.clear();
}


auto TextLoggerUncompressed::flush(EID producerEID, TID producerTID, Addr start, Addr end,
                                   EID eid, TID tid) -> void
{
    fmt::format_to(std::back_inserter(logMsg),
                   "{},{} # {} {} {:#x} {:#x}",
                   eid,
                   tid,
                   producerTID,
                   producerEID,
                   start,
                   end);

    logger->info(logMsg);
    logMsg.clear();
}


auto TextLoggerUncompressed::flush(unsigned char syncType, unsigned numArgs, Addr *syncArgs,
                                   EID eid, TID tid) -> void
{
    flushSyncEvent(syncType, numArgs, syncArgs, eid, tid, logger);
}


auto TextLoggerUncompressed::instrMarker(int limit) -> void
{
    flushInstrMarker(limit, logger);
}


auto flushPthread(std::string filePath,
                  ThreadList newThreadsInOrder,
                  SpawnList threadSpawns,
                  BarrierList barrierParticipants) -> void
{
    auto loggerPair = prism::getFileLogger(filePath);
    auto logger = std::move(loggerPair.first);
    info("Flushing thread metadata to: " + logger->name());

    /* The order the threads were seen SHOULD match to
     * the order of thread_t values of the pthread_create
     * calls. For example, with the valgrind frontend,
     * the --fair-sched=yes option should make sure each
     * thread is switched to in the order they were created.
     *
     * NOTE that there is always one MORE thread create,
     * because the initial thread is not created by a thread spawning API call */
    assert(threadSpawns.size() == newThreadsInOrder.size()-1);

    unsigned idx = 1;
    for (auto &p : threadSpawns)
    {
        /* SynchroTraceSim only supports threads
         * that were spawned from the original thread */
        if (p.first == 1)
            logger->info("##{},{}", p.second, newThreadsInOrder[idx]);
        ++idx;
    }

    for (auto &p: barrierParticipants)
    {
        std::string str = fmt::format("**{}", p.first);
        for (auto tid : p.second)
            fmt::format_to(std::back_inserter(str), ",{}", tid);

        logger->info(str);
    }

    logger->flush();
    prism::blockingFlushAndDeleteLogger(logger);
}


auto flushStats(std::string filePath, ThreadStatMap allThreadsStats) -> void
{
    auto loggerPair = prism::getFileLogger(filePath);
    auto logger = std::move(loggerPair.first);
    info("Flushing statistics to: " + logger->name());

    StatCounter totalInstrs{0};
    for (auto &p : allThreadsStats)
    {
        /* per thread */
        TID tid = p.first;
        Stats stats = p.second.getTotalStats();

        logger->info("thread : {}",  tid);
        logger->info("\tIOPS  : {}", std::get<IOP>(stats));
        logger->info("\tFLOPS : {}", std::get<FLOP>(stats));
        logger->info("\tReads : {}", std::get<READ>(stats));
        logger->info("\tWrites: {}", std::get<WRITE>(stats));

        totalInstrs += std::get<INSTR>(stats);

        AllBarriersStats barrierStatsForThread = p.second.getBarrierStats();
        for (auto &p : barrierStatsForThread)
        {
            /* per barrier region */
            logger->info("\tBarrier: {}",         p.first);
            logger->info("\t\tIOPs: {}",          p.second.iops);
            logger->info("\t\tFLOPs: {}",         p.second.flops);
            logger->info("\t\tInstrs: {}",        p.second.instrs);
            logger->info("\t\tMemAccesses: {}",   p.second.memAccesses);
            logger->info("\t\tCommunication: {}", p.second.communication);
            logger->info("\t\tlocks: {}",         p.second.locks);
            logger->info("\t\tIOPs/Mem: {}",      p.second.iopsPerMemAccess());
            logger->info("\t\tFLOPs/Mem: {}",     p.second.flopsPerMemAccess());
            logger->info("\t\tlocks/OPs: {}",     p.second.locksPerIopsPlusFlops());
        }

        AllLocksStats lockStatsForThread = p.second.getLockStats();
        for (auto &p : lockStatsForThread)
        {
            /* per lock region */
            logger->info("\tLock: {}",            p.first);
            logger->info("\t\tIOPs: {}",          p.second.iops);
            logger->info("\t\tFLOPs: {}",         p.second.flops);
            logger->info("\t\tInstrs: {}",        p.second.instrs);
            logger->info("\t\tMemAccesses: {}",   p.second.memAccesses);
            logger->info("\t\tCommunication: {}", p.second.communication);
        }
    }

    logger->info("Barrier statistics for all threads:");
    AllBarriersStats mergedBarrierStats;
    for (auto &p : allThreadsStats)
        BarrierMerge::merge(p.second.getBarrierStats(), mergedBarrierStats);
    for (auto &p : mergedBarrierStats)
    {
        /* per barrier region, all threads */
        logger->info("Barrier: ",       p.first);
        logger->info("\tIOPs: ",        p.second.iops);
        logger->info("\tFLOPs: ",       p.second.flops);
        logger->info("\tInstrs: ",      p.second.instrs);
        logger->info("\tMemAccesses: ", p.second.memAccesses);
        logger->info("\tlocks: ",       p.second.locks);
        logger->info("\tIOPs/Mem: ",    p.second.iopsPerMemAccess());
        logger->info("\tFLOPs/Mem: ",   p.second.flopsPerMemAccess());
        logger->info("\tlocks/OPs: ",   p.second.locksPerIopsPlusFlops());
    }

    logger->info("Total instructions for all threads: {}", totalInstrs);
    logger->flush();
    prism::blockingFlushAndDeleteLogger(logger);
}

}; //end namespace STGen

#include "TextLogger.hpp"

namespace STGen
{

namespace
{

template <typename I>
inline auto n2hexstr(char (&hexStr)[sizeof(I)*2+3], const I &w) -> const char*
{
    /* For speed, adapted from http://stackoverflow.com/a/33447587
     * hexStr MUST be at least (sizeof(I) * 2 + 3)
     * Returns the beginning location of the hex string,
     * since the supplied char* will be truncated:
     *     8BADF00D0xDEADBEEF\0
     *             |
     *             +----- the hex string starts here at 0x */
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

    /* TODO MDL20170309
     * If 'w' is '0' then this returns the full size hex string instead of just "0x0".
     * We can truncate the string to "0x0" to save space, but afaik this is very rare */
    hexStr[hexMSB - 2] = '0';
    hexStr[hexMSB - 1] = 'x';
    hexStr[hexLen + 2] = '\0';
    return hexStr + (hexMSB-2);
}


auto flushSyncEvent(unsigned char syncType, unsigned numArgs, Addr *syncArgs,
                    EID eid, TID tid,
                    std::shared_ptr<spdlog::logger> &logger) -> void
{
    assert(numArgs > 0);
    char hexStr[sizeof(Addr)*2+3];
    std::string logMsg;
    logMsg += std::to_string(eid);
    logMsg += ",";
    logMsg += std::to_string(tid);
    logMsg += ",pth_ty:";
    logMsg += std::to_string(syncType);
    logMsg += "^";
    logMsg += n2hexstr(hexStr, syncArgs[0]);
    for (unsigned i=1; i<numArgs; ++i)
    {
        logMsg += "&";
        logMsg += n2hexstr(hexStr, syncArgs[i]);
    }
    logger->info(logMsg);
}


auto flushInstrMarker(int limit, std::shared_ptr<spdlog::logger> &logger) -> void
{
    logger->info("! " + std::to_string(limit));
}

}; //end namespace


TextLoggerCompressed::TextLoggerCompressed(TID tid, std::string outputPath)
{
    assert(tid >= 1);

    /* XXX PERFORMANCE: modify this to reduce memory in STGen.
     * Buffers for asynchronous I/O */
    spdlog::set_async_mode(1 << 14);

    auto filePath = outputPath + "/sigil.events.out-" + std::to_string(tid) + ".gz";
    std::tie(logger, gzfile) = sigil2::getGzLogger(filePath);
}


TextLoggerCompressed::~TextLoggerCompressed()
{
    sigil2::blockingFlushAndDeleteLogger(logger);
    /* gzofstream destructor closes gzfile  */
}


auto TextLoggerCompressed::flush(const STCompEventCompressed& ev, EID eid, TID tid) -> void
{
    /* http://stackoverflow.com/a/18892355 */
    logMsg += std::to_string(eid);
    logMsg += ",";
    logMsg += std::to_string(tid);
    logMsg += ",";
    logMsg += std::to_string(ev.iops);
    logMsg += ",";
    logMsg += std::to_string(ev.flops);
    logMsg += ",";
    logMsg += std::to_string(ev.reads);
    logMsg += ",";
    logMsg += std::to_string(ev.writes);

    char hexStr[sizeof(Addr)*2+3];
    for (auto &p : ev.uniqueWriteAddrs.get())
    {
        assert(p.first <= p.second);
        logMsg += " $ ";
        logMsg += n2hexstr(hexStr, p.first);
        logMsg += " ";
        logMsg += n2hexstr(hexStr, p.second);
    }

    for (auto &p : ev.uniqueReadAddrs.get())
    {
        assert(p.first <= p.second);
        logMsg += " * ";
        logMsg += n2hexstr(hexStr, p.first);
        logMsg += " ";
        logMsg += n2hexstr(hexStr, p.second);
    }

    logger->info(logMsg);
    logMsg.clear();
}


auto TextLoggerCompressed::flush(const STCommEventCompressed& ev, EID eid, TID tid) -> void
{
    assert(ev.comms.empty() == false);
    logMsg += std::to_string(eid);
    logMsg += ",";
    logMsg += std::to_string(tid);

    char hexStr[sizeof(Addr)*2+3];
    for (auto &edge : ev.comms)
    {
        for (auto &p : std::get<2>(edge).get())
        {
            assert(p.first <= p.second);
            logMsg += " # ";
            logMsg += std::to_string(std::get<0>(edge));
            logMsg += " ";
            logMsg += std::to_string(std::get<1>(edge));
            logMsg += " ";
            logMsg += n2hexstr(hexStr, p.first);
            logMsg += " ";
            logMsg += n2hexstr(hexStr, p.second);
        }
    }

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


TextLoggerUncompressed::TextLoggerUncompressed(TID tid, std::string outputPath)
{
    assert(tid >= 1);

    /* XXX PERFORMANCE: modify this to reduce memory in STGen.
     * Buffers for asynchronous I/O */
    spdlog::set_async_mode(1 << 14);

    auto filePath = outputPath + "/sigil.events.out-" + std::to_string(tid) + ".gz";
    std::tie(logger, gzfile) = sigil2::getGzLogger(filePath);
}


TextLoggerUncompressed::~TextLoggerUncompressed()
{
    sigil2::blockingFlushAndDeleteLogger(logger);
    /* gzofstream destructor closes gzfile  */
}


auto TextLoggerUncompressed::flush(StatCounter iops, StatCounter flops,
                                   STCompEventUncompressed::MemType type, Addr start, Addr end,
                                   EID eid, TID tid) -> void
{
    /* http://stackoverflow.com/a/18892355 */
    logMsg += std::to_string(eid);
    logMsg += ",";
    logMsg += std::to_string(tid);
    logMsg += ",";
    logMsg += std::to_string(iops);
    logMsg += ",";
    logMsg += std::to_string(flops);

    char hexStr[sizeof(Addr)*2+3];
    switch (type)
    {
    /* only one of
     *  - no reads/writes,
     *  - one read,
     *  - one write
     * possible in uncompressed mode */
    case STCompEventUncompressed::MemType::READ:
        logMsg += ",1,0 * ";
        logMsg += n2hexstr(hexStr, start);
        logMsg += " ";
        logMsg += n2hexstr(hexStr, end);
        break;
    case STCompEventUncompressed::MemType::WRITE:
        logMsg += ",0,1 $ ";
        logMsg += n2hexstr(hexStr, start);
        logMsg += " ";
        logMsg += n2hexstr(hexStr, end);
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
    logMsg += std::to_string(eid);
    logMsg += ",";
    logMsg += std::to_string(tid);

    logMsg += " # ";
    logMsg += std::to_string(producerTID);
    logMsg += " ";
    logMsg += std::to_string(producerEID);
    logMsg += " ";

    char hexStr[sizeof(Addr)*2+3];
    logMsg += n2hexstr(hexStr, start);
    logMsg += " ";
    logMsg += n2hexstr(hexStr, end);

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
    auto loggerPair = sigil2::getFileLogger(filePath);
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
            logger->info("##" + std::to_string(p.second) +
                         "," + std::to_string(newThreadsInOrder[idx]));
        ++idx;
    }

    for (auto &p: barrierParticipants)
    {
        std::ostringstream ss;
        ss << "**" << p.first;

        for (auto tid : p.second)
            ss << "," << tid;

        logger->info(ss.str());
    }

    logger->flush();
    sigil2::blockingFlushAndDeleteLogger(logger);
}


auto flushStats(std::string filePath, ThreadStatMap allThreadsStats) -> void
{
    auto loggerPair = sigil2::getFileLogger(filePath);
    auto logger = std::move(loggerPair.first);
    info("Flushing statistics to: " + logger->name());

    StatCounter totalInstrs{0};
    for (auto &p : allThreadsStats)
    {
        /* per thread */
        TID tid = p.first;
        Stats stats = p.second.getTotalStats();

        logger->info("thread : " + std::to_string(tid));
        logger->info("\tIOPS  : " + std::to_string(std::get<IOP>(stats)));
        logger->info("\tFLOPS : " + std::to_string(std::get<FLOP>(stats)));
        logger->info("\tReads : " + std::to_string(std::get<READ>(stats)));
        logger->info("\tWrites: " + std::to_string(std::get<WRITE>(stats)));

        totalInstrs += std::get<INSTR>(stats);

        AllBarriersStats barrierStatsForThread = p.second.getBarrierStats();
        for (auto &p : barrierStatsForThread)
        {
            /* per barrier region */
            logger->info("\tBarrier: " + std::to_string(p.first));
            logger->info("\t\tIOPs: " + std::to_string(p.second.iops));
            logger->info("\t\tFLOPs: " + std::to_string(p.second.flops));
            logger->info("\t\tInstrs: " + std::to_string(p.second.instrs));
            logger->info("\t\tMemAccesses: " + std::to_string(p.second.memAccesses));
            logger->info("\t\tCommunication: " + std::to_string(p.second.communication));
            logger->info("\t\tlocks: " + std::to_string(p.second.locks));
            logger->info("\t\tIOPs/Mem: " + std::to_string(p.second.iopsPerMemAccess()));
            logger->info("\t\tFLOPs/Mem: " + std::to_string(p.second.flopsPerMemAccess()));
            logger->info("\t\tlocks/OPs: " + std::to_string(p.second.locksPerIopsPlusFlops()));
        }

        AllLocksStats lockStatsForThread = p.second.getLockStats();
        for (auto &p : lockStatsForThread)
        {
            /* per lock region */
            logger->info("\tLock: " + std::to_string(p.first));
            logger->info("\t\tIOPs: " + std::to_string(p.second.iops));
            logger->info("\t\tFLOPs: " + std::to_string(p.second.flops));
            logger->info("\t\tInstrs: " + std::to_string(p.second.instrs));
            logger->info("\t\tMemAccesses: " + std::to_string(p.second.memAccesses));
            logger->info("\t\tCommunication: " + std::to_string(p.second.communication));
        }
    }

    logger->info("Barrier statistics for all threads:");
    AllBarriersStats mergedBarrierStats;
    for (auto &p : allThreadsStats)
        BarrierMerge::merge(p.second.getBarrierStats(), mergedBarrierStats);
    for (auto &p : mergedBarrierStats)
    {
        /* per barrier region, all threads */
        logger->info("Barrier: " + std::to_string(p.first));
        logger->info("\tIOPs: " + std::to_string(p.second.iops));
        logger->info("\tFLOPs: " + std::to_string(p.second.flops));
        logger->info("\tInstrs: " + std::to_string(p.second.instrs));
        logger->info("\tMemAccesses: " + std::to_string(p.second.memAccesses));
        logger->info("\tlocks: " + std::to_string(p.second.locks));
        logger->info("\tIOPs/Mem: " + std::to_string(p.second.iopsPerMemAccess()));
        logger->info("\tFLOPs/Mem: " + std::to_string(p.second.flopsPerMemAccess()));
        logger->info("\tlocks/OPs: " + std::to_string(p.second.locksPerIopsPlusFlops()));
    }

    logger->info("Total instructions for all threads: " + std::to_string(totalInstrs));
    logger->flush();
    sigil2::blockingFlushAndDeleteLogger(logger);
}

}; //end namespace STGen

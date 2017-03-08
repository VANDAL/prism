#ifndef STGEN_TEXT_LOGGER_H
#define STGEN_TEXT_LOGGER_H

#include "BarrierMerge.hpp"
#include "STEvent.hpp"
#include "STTypes.hpp"
#include "Sigil2/SigiLog.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/ostream_sink.h"
#include "zfstream.h"
#include <fstream>
#include <sstream>

using SigiLog::info;
namespace STGen
{

/* For speed, adapted from http://stackoverflow.com/a/33447587
 * hexStr MUST be at least (sizeof(I) * 2 + 3)
 * Returns the beginning location of the hex string,
 * since the supplied char* will be truncated:
 *     8BADF00D0xDEADBEEF\0
 *             |
 *             +----- the hex string starts here at 0x */
template <typename I>
inline auto n2hexstr(char (&hexStr)[sizeof(I)*2+3], const I &w) -> const char*
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

    /* TODO if 'w' is '0' then this returns the full size hex string, instead of just "0x0" */
    /* Truncate the string if it doesn't need the full width
     * This will save space in the ASCII formatted trace */
    hexStr[hexMSB - 2] = '0';
    hexStr[hexMSB - 1] = 'x';
    hexStr[hexLen + 2] = '\0';
    return hexStr + (hexMSB-2);
}

class TextLogger
{
    /* Uses spdlog logging library to asynchronously log to a text file.
     * The format is a custom format.
     * Each new logger writes to a new file */
  public:
    TextLogger(TID tid, std::string outputPath)
    {
        assert(tid >= 1);

        /* XXX PERFORMANCE: modify this to reduce memory in STGen.
         * Buffers for asynchronous I/O */
        spdlog::set_async_mode(1 << 14);

        auto filePath = outputPath + "/sigil.events.out-" + std::to_string(tid) + ".gz";
        std::tie(logger, gzfile) = getGzLogger(filePath);
    }

    TextLogger(const TextLogger& other) = delete;
    ~TextLogger()
    {
        blockingLoggerFlush(logger);
        /* gzofstream destructor closes gzfile  */
    }

    /* Log a SynchroTrace aggregate Compute Event */
    auto flush(const STCompEvent& ev, const EID eid, const TID tid) -> void
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

    /* Log a SynchroTrace Communication Event */
    auto flush(const STCommEvent& ev, const EID eid, const TID tid) -> void
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

    /* Log a SynchroTrace Synchronization Event */
    auto flush(const unsigned char syncType, const Addr syncAddr,
               const EID eid, const TID tid) -> void
    {
        char hexStr[sizeof(Addr)*2+3];
        logMsg += std::to_string(eid);
        logMsg += ",";
        logMsg += std::to_string(tid);
        logMsg += ",pth_ty:";
        logMsg += std::to_string(syncType);
        logMsg += "^";
        logMsg += n2hexstr(hexStr, syncAddr);
        logger->info(logMsg);
        logMsg.clear();
    }

    /* Place a marker in the trace after 'limit' instructions */
    auto instrMarker(int limit) -> void
    {
        logger->info("! " + std::to_string(limit));
    }

    static auto flushPthread(std::string filePath,
                             ThreadList newThreadsInOrder,
                             SpawnList threadSpawns,
                             BarrierList barrierParticipants) -> void
    {
        auto loggerPair = getFileLogger(filePath);
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
        blockingLoggerFlush(logger);
    }

    static auto flushStats(std::string filePath, ThreadStatMap allThreadsStats) -> void
    {
        auto loggerPair = getFileLogger(filePath);
        auto logger = std::move(loggerPair.first);
        info("Flushing statistics to: " + logger->name());

        StatCounter totalInstrs{0};
        for (auto &p : allThreadsStats)
        {
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
                /* Log per thread, per barrier */
                logger->info("\tBarrier: " + std::to_string(p.first));
                logger->info("\t\tIOPs: " + std::to_string(p.second.iops));
                logger->info("\t\tFLOPs: " + std::to_string(p.second.flops));
                logger->info("\t\tInstrs: " + std::to_string(p.second.instrs));
                logger->info("\t\tMemAccesses: " + std::to_string(p.second.memAccesses));
                logger->info("\t\tlocks: " + std::to_string(p.second.locks));
                logger->info("\t\tIOPs/Mem: " + std::to_string(p.second.iopsPerMemAccess()));
                logger->info("\t\tFLOPs/Mem: " + std::to_string(p.second.flopsPerMemAccess()));
                logger->info("\t\tlocks/OPs: " + std::to_string(p.second.locksPerIopsPlusFlops()));
            }
        }

        /* Merge barrier statistics across threads */
        AllBarriersStats mergedBarrierStats;
        for (auto &p : allThreadsStats)
            BarrierMerge::merge(p.second.getBarrierStats(), mergedBarrierStats);

        /* Print merged barrier statistics */
        logger->info("Barrier statistics for all threads:");
        for (auto &p : mergedBarrierStats)
        {
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
        blockingLoggerFlush(logger);
    }


  private:
    /* XXX REMEMBER: the file stream needs to be returned
     * with the logger to extend the life of the stream */
    static auto getFileLogger(std::string filePath)
        -> std::pair<std::shared_ptr<spdlog::logger>, std::shared_ptr<std::ofstream>>
    {
        auto file = std::make_shared<std::ofstream>(filePath, std::ios::trunc | std::ios::out);
        if (file->fail() == true)
            fatal("Failed to open: " + filePath);
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(*file);
        auto logger = spdlog::create(filePath, {sink});
        logger->set_pattern("%v");
        return std::make_pair(logger, file);
    }

    static auto getGzLogger(std::string filePath)
        -> std::pair<std::shared_ptr<spdlog::logger>, std::shared_ptr<gzofstream>>
    {
        auto gzfile = std::make_shared<gzofstream>(filePath.c_str(), std::ios::trunc | std::ios::out);
        if (gzfile->fail() == true)
            fatal("Failed to open: " + filePath);
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(*gzfile);
        auto logger = spdlog::create(filePath, {sink});
        logger->set_pattern("%v");
        return std::make_pair(logger, gzfile);
    }

    static auto blockingLoggerFlush(std::shared_ptr<spdlog::logger> &logger) -> void
    {
        /* Make sure all other shared_ptrs are destroyed, as well.
         * Might be annoying to throw here, but it'll save debugging headaches.
         * Expect 2 reference counts:
         * - the passed in reference
         * - spdlog global registry  */
        if (logger.use_count() > 2)
            throw std::invalid_argument("tried to flush spdlog logger "
                                        "while still in use somewhere: " + logger->name());

        /* Destructor calls a blocking flush in case of asynchronous logging. */
        logger->flush();
        spdlog::drop(logger->name()); // remove from global registry
        logger.reset();
    }


    std::string logMsg; // reuse to save on heap allocations space
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<gzofstream> gzfile;
};

}; //end namespace STGen

#endif

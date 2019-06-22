#include "TextLoggerV2.hpp"
#include "spdlog/fmt/fmt.h"

namespace STGen
{

namespace
{

auto flushSyncEvent(unsigned char syncType,
                    unsigned numArgs,
                    Addr *syncArgs,
                    EID eid,
                    TID tid,
                    std::shared_ptr<spdlog::logger> &logger) -> void
{
    (void)eid;
    (void)tid;

    assert(numArgs > 0);

    std::string logMsg = fmt::format("^ {:d}^{:#x}", syncType, syncArgs[0]);
    for (unsigned i = 1; i < numArgs; ++i)
        fmt::format_to(std::back_inserter(logMsg), "&{:#x}", syncArgs[i]);

    logger->info(logMsg);
}


auto flushInstrMarker(int limit, std::shared_ptr<spdlog::logger> &logger) -> void
{
    logger->info("! {}", limit);
}

}; //end namespace


TextLoggerV2Compressed::TextLoggerV2Compressed(TID tid, const std::string& outputPath)
{
    assert(tid >= 1);

    std::string filePath = fmt::format("{}/sigil.events.out-{}.gz", outputPath, tid);
    std::tie(logger, gzfile) =
        prism::getFileLogger<spdlog::async_factory, gzofstream>(std::move(filePath));
}


TextLoggerV2Compressed::~TextLoggerV2Compressed()
{
    prism::blockingFlushAndDeleteLogger(logger);
    /* gzofstream destructor closes gzfile  */
}


auto TextLoggerV2Compressed::flush(const STCompEventCompressed& ev, EID eid, TID tid) -> void
{
    (void)eid;
    (void)tid;

    fmt::format_to(std::back_inserter(logMsg), "@ {},{},{},{} ",
                   ev.iops, ev.flops, ev.reads, ev.writes);

    for (auto &p : ev.uniqueWriteAddrs.get())
    {
        assert(p.first <= p.second);
        fmt::format_to(std::back_inserter(logMsg), "$ {:#x} {:#x} ", p.first, p.second);
    }

    for (auto &p : ev.uniqueReadAddrs.get())
    {
        assert(p.first <= p.second);
        fmt::format_to(std::back_inserter(logMsg), "* {:#x} {:#x} ", p.first, p.second);
    }

    logger->info(logMsg);
    logMsg.clear();
}


auto TextLoggerV2Compressed::flush(const STCommEventCompressed& ev, EID eid, TID tid) -> void
{
    (void)eid;
    (void)tid;

    assert(ev.comms.empty() == false);

    for (auto &edge : ev.comms)
        for (auto &p : std::get<2>(edge).get())
            fmt::format_to(std::back_inserter(logMsg), "# {} {} {:#x} {:#x} ",
                           std::get<0>(edge), std::get<1>(edge), p.first, p.second);

    logger->info(logMsg);
    logMsg.clear();
}


auto TextLoggerV2Compressed::flush(unsigned char syncType,
                                   unsigned numArgs,
                                   Addr *syncArgs,
                                   EID eid,
                                   TID tid) -> void
{
    flushSyncEvent(syncType, numArgs, syncArgs, eid, tid, logger);
}


auto TextLoggerV2Compressed::instrMarker(int limit) -> void
{
    flushInstrMarker(limit, logger);
}


TextLoggerV2Uncompressed::TextLoggerV2Uncompressed(TID tid, const std::string& outputPath)
{
    assert(tid >= 1);

    std::string filePath = fmt::format("{}/sigil.events.out-{}.gz", outputPath, tid);
    std::tie(logger, gzfile) =
        prism::getFileLogger<spdlog::async_factory, gzofstream>(std::move(filePath));
}


TextLoggerV2Uncompressed::~TextLoggerV2Uncompressed()
{
    prism::blockingFlushAndDeleteLogger(logger);
    /* gzofstream destructor closes gzfile  */
}


auto TextLoggerV2Uncompressed::flush(StatCounter iops,
                                     StatCounter flops,
                                     STCompEventUncompressed::MemType type,
                                     Addr start,
                                     Addr end,
                                     EID eid,
                                     TID tid) -> void
{
    (void)eid;
    (void)tid;

    fmt::format_to(std::back_inserter(logMsg), "@ {},{}", iops, flops);

    switch (type)
    {
    /* only one of
     *  - no reads/writes,
     *  - one read,
     *  - one write
     * possible in uncompressed mode */
    case STCompEventUncompressed::MemType::READ:
        fmt::format_to(std::back_inserter(logMsg), ",1,0 * {:#x} {:#x} ", start, end);
        break;
    case STCompEventUncompressed::MemType::WRITE:
        fmt::format_to(std::back_inserter(logMsg), ",0,1 $ {:#x} {:#x} ", start, end);
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


auto TextLoggerV2Uncompressed::flush(EID producerEID,
                                     TID producerTID,
                                     Addr start,
                                     Addr end,
                                     EID eid,
                                     TID tid) -> void
{
    (void)eid;
    (void)tid;

    fmt::format_to(std::back_inserter(logMsg),
                   "# {} {} {:#x} {:#x} ",
                   producerTID,
                   producerEID,
                   start,
                   end);

    logger->info(logMsg);
    logMsg.clear();
}


auto TextLoggerV2Uncompressed::flush(unsigned char syncType,
                                     unsigned numArgs,
                                     Addr *syncArgs,
                                     EID eid,
                                     TID tid) -> void
{
    flushSyncEvent(syncType, numArgs, syncArgs, eid, tid, logger);
}


auto TextLoggerV2Uncompressed::instrMarker(int limit) -> void
{
    flushInstrMarker(limit, logger);
}

}; //end namespace STGen

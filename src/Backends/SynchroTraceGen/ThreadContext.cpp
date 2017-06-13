#include "ThreadContext.hpp"
#include "TextLogger.hpp"
#include "CapnLogger.hpp"
#include "NullLogger.hpp"

namespace STGen
{

//-----------------------------------------------------------------------------
/** Compressed ThreadContext **/
ThreadContextCompressed::ThreadContextCompressed(TID tid,
                                                 unsigned primsPerStCompEv,
                                                 std::string outputPath,
                                                 std::string loggerType)
    : tid(tid)
    , primsPerStCompEv(primsPerStCompEv)
{
    /* current shadow memory limit */
    assert(tid <= 128);
    assert(primsPerStCompEv > 0 && primsPerStCompEv <= 100);

    logger = getLogger(tid, outputPath, loggerType);
}


ThreadContextCompressed::~ThreadContextCompressed()
{
    compFlushIfActive();
    commFlushIfActive();
}


auto ThreadContextCompressed::getStats() const -> PerThreadStats
{
    return stats;
}


auto ThreadContextCompressed::onIop() -> void
{
    commFlushIfActive();
    stComp.incIOP();
    stats.incIOPs();
}


auto ThreadContextCompressed::onFlop() -> void
{
    commFlushIfActive();
    stComp.incFLOP();
    stats.incFLOPs();
}


auto ThreadContextCompressed::onRead(Addr start, Addr bytes) -> void
{
    bool isCommEdge = false;

    /* Each byte of the read may have been touched by a different thread,
     * so check the reader/writer pair for each byte */
    for (Addr i = 0; i < bytes; ++i)
    {
        Addr addr = start + i;
        try
        {
            TID writer = shadow.getWriterTID(addr);
            bool isReader= shadow.isReaderTID(addr, tid);

            if (isReader == false)
                shadow.updateReader(addr, 1, tid);

            if ((isReader == false) && (writer != tid) && (writer != SO_UNDEF))
            {
                isCommEdge = true;
                stComm.addEdge(writer, shadow.getWriterEID(addr), addr);
            }
            else /*local load, comp event*/
            {
                /* treat a read/write to an address with
                 * UNDEF thread as a local compute event */
                stComp.updateReads(addr, 1);
            }
        }
        catch(std::out_of_range &e)
        {
            /* treat as a local event */
            warn(e.what());
            stComp.updateReads(addr, 1);
        }
    }

    /* A situation when a singular memory event is both a communication edge
     * and a local thread read is rare and not robustly accounted for.
     * A single address that is a communication edge counts the whole event
     * as a communication event, and not as part of a computation event
     * Some loss of granularity can occur in this situation */
    if (isCommEdge == false)
    {
        commFlushIfActive();
        stComp.incReads();
        stats.incComm();
    }
    else
    {
        compFlushIfActive();
    }

    checkCompFlushLimit();
    stats.incReads();
}


auto ThreadContextCompressed::onWrite(Addr start, Addr bytes) -> void
{
    stComp.incWrites();
    stComp.updateWrites(start, bytes);

    try
    {
        shadow.updateWriter(start, bytes, tid, events);
    }
    catch(std::out_of_range &e)
    {
        warn(e.what());
    }

    checkCompFlushLimit();
    stats.incWrites();
}


auto ThreadContextCompressed::onSync(unsigned char syncType,
                                     unsigned numArgs, Addr *syncArgs) -> void
{
    compFlushIfActive();
    commFlushIfActive();

    if (INCR_EID_OVERFLOW(events))
        fatal("Event ID overflow detected in thread: " + std::to_string(tid));

    stats.incSyncs(syncType, numArgs, syncArgs);
    logger->flush(syncType, numArgs, syncArgs, events, tid);
}


auto ThreadContextCompressed::onInstr() -> void
{
    stats.incInstrs();

    /* add marker every 2**N instructions */
    constexpr int limit = 1 << 12;
    if (((limit-1) & stats.getTotalInstrs()) == 0)
        logger->instrMarker(limit);
}


auto ThreadContextCompressed::checkCompFlushLimit() -> void
{
    if ((stComp.writes >= primsPerStCompEv) || (stComp.reads >= primsPerStCompEv))
        compFlushIfActive();

    assert(stComp.isActive == false ||
           ((stComp.writes < primsPerStCompEv) && (stComp.reads < primsPerStCompEv)));
}


auto ThreadContextCompressed::compFlushIfActive() -> void
{
    if (stComp.isActive == true)
    {
        logger->flush(stComp, events, tid);
        stComp.reset();
        if (INCR_EID_OVERFLOW(events))
            fatal("Event ID overflow detected in thread: " + std::to_string(tid));
    }
    assert(stComp.isActive == false);
}


auto ThreadContextCompressed::commFlushIfActive() -> void
{
    if (stComm.isActive == true)
    {
        logger->flush(stComm, events, tid);
        stComm.reset();
        if (INCR_EID_OVERFLOW(events))
            fatal("Event ID overflow detected in thread: " + std::to_string(tid));
    }
    assert(stComm.isActive == false);
}


auto ThreadContextCompressed::flushAll() -> void
{
    compFlushIfActive();
    commFlushIfActive();
}


auto ThreadContextCompressed::getLogger(TID tid, std::string outputPath,
                                        std::string loggerType) -> LogPtr
{
    if (loggerType == "text")
        return std::make_unique<TextLoggerCompressed>(tid, outputPath);
    else if (loggerType == "capnp")
        return std::make_unique<CapnLoggerCompressed>(tid, outputPath);
    else if (loggerType == "null")
        return std::make_unique<NullLogger>(tid, outputPath);
    else
        fatal("Invalid logger type");
}


//-----------------------------------------------------------------------------
/** Uncompressed ThreadContext **/
ThreadContextUncompressed::ThreadContextUncompressed(TID tid,
                                                     unsigned primsPerStCompEv,
                                                     std::string outputPath,
                                                     std::string loggerType)
    : tid(tid)
    , primsPerStCompEv(primsPerStCompEv)
{
    /* current shadow memory limit */
    assert(tid <= 128);
    assert(primsPerStCompEv > 0 && primsPerStCompEv <= 100);

    logger = getLogger(tid, outputPath, loggerType);
}


ThreadContextUncompressed::~ThreadContextUncompressed()
{
    compFlushIfActive();
}


auto ThreadContextUncompressed::getStats() const -> PerThreadStats
{
    return stats;
}


auto ThreadContextUncompressed::onIop() -> void
{
    stComp.incIOP();
    stats.incIOPs();
}


auto ThreadContextUncompressed::onFlop() -> void
{
    stComp.incFLOP();
    stats.incFLOPs();
}


auto ThreadContextUncompressed::onRead(Addr start, Addr bytes) -> void
{
    /* Each byte of the read may have been touched by a different thread
     * If one byte was touched by another thread, consider the entire read
     * a communication event, from that thread without checking the rest of the
     * bytes. The case where a single 'read' was written to by multiple threads
     * is rare in our use case of user-space synchronization e.g. spinlocks.
     *
     * TODO MDL20170321 Create parity with compressed read event */

    bool isCommEdge = false;
    TID producerTID{0};
    EID producerEID{0};

    for (Addr i = 0; i < bytes; ++i)
    {
        Addr addr = start + i;
        try
        {
            TID writer = shadow.getWriterTID(addr);
            bool isReader= shadow.isReaderTID(addr, tid);

            if (isReader == false)
                shadow.updateReader(addr, 1, tid);

            if /*comm edge*/((isReader == false) && (writer != tid) && (writer != SO_UNDEF))
            {
                isCommEdge = true;
                producerTID = writer;
                producerEID = shadow.getWriterEID(addr);
                break;
            }
        }
        catch(std::out_of_range &e)
        {
            /* XXX treat as a local event */
            warn(e.what());
        }
    }

    if (isCommEdge == true)
        commFlush(producerEID, producerTID, start, start+bytes-1);
    else
        compFlush(STCompEventUncompressed::MemType::READ, start, start+bytes-1);

    stats.incReads();
}


auto ThreadContextUncompressed::onWrite(Addr start, Addr bytes) -> void
{
    compFlush(STCompEventUncompressed::MemType::WRITE, start, start+bytes-1);

    try
    {
        shadow.updateWriter(start, bytes, tid, events);
    }
    catch(std::out_of_range &e)
    {
        warn(e.what());
    }

    stats.incWrites();
}


auto ThreadContextUncompressed::onSync(unsigned char syncType,
                                       unsigned numArgs, Addr *syncArgs) -> void
{
    compFlushIfActive();

    if (INCR_EID_OVERFLOW(events))
        fatal("Event ID overflow detected in thread: " + std::to_string(tid));

    stats.incSyncs(syncType, numArgs, syncArgs);
    logger->flush(syncType, numArgs, syncArgs, events, tid);
}


auto ThreadContextUncompressed::onInstr() -> void
{
    stats.incInstrs();

    /* add marker every 2**N instructions */
    constexpr int limit = 1 << 12;
    if (((limit-1) & stats.getTotalInstrs()) == 0)
        logger->instrMarker(limit);
}


auto ThreadContextUncompressed::compFlush(STCompEventUncompressed::MemType type,
                                          Addr start, Addr end) -> void
{
    logger->flush(stComp.iops, stComp.flops, type, start, end, events, tid);
    stComp.reset();
    if (INCR_EID_OVERFLOW(events))
        fatal("Event ID overflow detected in thread: " + std::to_string(tid));
    assert(stComp.isActive == false);
}


auto ThreadContextUncompressed::compFlushIfActive() -> void
{
    /* Flushing for reason other than memory access */

    if (stComp.isActive == true)
    {
        logger->flush(stComp.iops, stComp.flops,
                      STCompEventUncompressed::MemType::NONE, 0, 0, events, tid);
        stComp.reset();
        if (INCR_EID_OVERFLOW(events))
            fatal("Event ID overflow detected in thread: " + std::to_string(tid));
    }
    assert(stComp.isActive == false);
}


auto ThreadContextUncompressed::commFlush(EID producerEID, TID producerTID,
                                          Addr start, Addr end) -> void
{
    logger->flush(producerEID, producerTID, start, end, events, tid);
    if (INCR_EID_OVERFLOW(events))
        fatal("Event ID overflow detected in thread: " + std::to_string(tid));
}


auto ThreadContextUncompressed::flushAll() -> void
{
    compFlushIfActive();
}


auto ThreadContextUncompressed::getLogger(TID tid, std::string outputPath,
                                          std::string loggerType) -> LogPtr
{
    if (loggerType == "text")
        return std::make_unique<TextLoggerUncompressed>(tid, outputPath);
    else if (loggerType == "capnp")
        return std::make_unique<CapnLoggerUncompressed>(tid, outputPath);
    else if (loggerType == "null")
        return std::make_unique<NullLogger>(tid, outputPath);
    else
        fatal("Invalid logger type");
}

}; //end namespace STGen

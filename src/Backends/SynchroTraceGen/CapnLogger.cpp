#include "CapnLogger.hpp"


//-----------------------------------------------------------------------------
/** CapnProto -> Gzip file **/
namespace kj
{

class GzOutputStream : public OutputStream
{
    /* Based off of FdOutputStream in capnproto library */
  public:
    explicit GzOutputStream(gzFile fz) : fz(fz) {}
    KJ_DISALLOW_COPY(GzOutputStream);
    ~GzOutputStream() noexcept(false) {}

    void write(const void* buffer, size_t size) override
    {
        int ret = gzwrite(fz, buffer, size);
        if (ret == 0)
            fatal("error writing gzipped capnproto serializaton");
    }

  private:
    gzFile fz;
};

}; //end namespace kj


namespace capnp
{

inline void writePackedMessageToGz(gzFile fz, MessageBuilder &message)
{
    /* Based off of writePackedMessageToFd in capnproto library */

    kj::GzOutputStream output(fz);
    writePackedMessage(output, message.getSegmentsForOutput());
}

}; //end nampespace capnp


//-----------------------------------------------------------------------------
/** CapnProto Logging **/
namespace STGen
{

namespace
{
/* Common between compressed/uncompressed */

template <typename Event, typename OrphanagePtr, typename OrphanList>
auto flushSyncEvent(unsigned char syncType, Addr syncAddr,
                    OrphanagePtr &orphanage, OrphanList &orphans) -> void
{
    auto orphan = orphanage->getOrphanage().template newOrphan<Event>();
    auto syncBuilder = orphan.get().initSync();

    /* translate type to CapnProto enum */
    syncBuilder.setId(syncAddr);
    switch (syncType)
    {
    case 1:
        syncBuilder.setType(Event::SyncType::LOCK);
        break;
    case 2:
        syncBuilder.setType(Event::SyncType::UNLOCK);
        break;
    case 3:
        syncBuilder.setType(Event::SyncType::SPAWN);
        break;
    case 4:
        syncBuilder.setType(Event::SyncType::JOIN);
        break;
    case 5:
        syncBuilder.setType(Event::SyncType::BARRIER);
        break;
    case 6:
        syncBuilder.setType(Event::SyncType::COND_WAIT);
        break;
    case 7:
        syncBuilder.setType(Event::SyncType::COND_SIGNAL);
        break;
    case 8:
        syncBuilder.setType(Event::SyncType::COND_BROADCAST);
        break;
    case 9:
        syncBuilder.setType(Event::SyncType::SPIN_LOCK);
        break;
    case 10:
        syncBuilder.setType(Event::SyncType::SPIN_UNLOCK);
        break;
    default:
        fatal("capnlogger encountered unhandled sync event");
    }

    orphans.emplace_back(std::move(orphan));
}


template <typename Event, typename OrphanagePtr, typename OrphanList>
auto flushInstrMarker(int limit, OrphanagePtr &orphanage, OrphanList &orphans) -> void
{
    auto orphan = orphanage->getOrphanage().template newOrphan<Event>();
    auto markerBuilder = orphan.get().initMarker();
    markerBuilder.setCount(limit);

    orphans.emplace_back(std::move(orphan));
}

template <typename EventStream, typename OrphanagePtr, typename OrphanList>
auto flushOrphans(OrphanagePtr flushedOrphanage, OrphanList flushedOrphans, gzFile fz) -> bool
{
    /* need to keep the orphanage alive until it's flushed */
    (void)flushedOrphanage;

    /* create the message now that we have a fixed length */
    ::capnp::MallocMessageBuilder message;
    auto eventStreamBuilder = message.initRoot<EventStream>();
    auto eventsBuilder = eventStreamBuilder.initEvents(flushedOrphans.size());

    for (unsigned i=0; i<flushedOrphans.size(); ++i)
    {
        auto reader = flushedOrphans[i].getReader();
        eventsBuilder.setWithCaveats(i, reader);
    }

    ::capnp::writePackedMessageToGz(fz, message);

    /* burn down the orphanage and orphans */
    flushedOrphans.clear(); /* kill orphans first,
                               otherwise we die when we run into the burning orphanage
                               to kill the orphans */
    return true;
}

}; //end namespace


//-----------------------------------------------------------------------------
/** Multiple reads/writes compressed **/
CapnLoggerCompressed::CapnLoggerCompressed(TID tid, std::string outputPath)
{
    assert(tid >= 1);

    /* initialize orphanage */
    orphanage = std::make_unique<::capnp::MallocMessageBuilder>();

    /* nothing being copied yet */
    doneCopying = std::async([]{return true;});

    auto filePath = (outputPath + "/sigil.events.out-" + std::to_string(tid) +
                     ".compressed.capn.bin.gz");
    fz = gzopen(filePath.c_str(), "wb");
    if (fz == NULL)
        fatal(std::string("opening gzfile: ") + strerror(errno));
}


CapnLoggerCompressed::~CapnLoggerCompressed()
{
    flushOrphansNow();
    int ret = gzclose(fz);
    if (ret != Z_OK)
        fatal(std::string("closing gzfile: ") + strerror(errno));
}


auto CapnLoggerCompressed::flush(const STCompEventCompressed& ev, EID eid, TID tid) -> void
{
    (void)eid;
    (void)tid;

    auto orphan = orphanage->getOrphanage().newOrphan<Event>();
    auto comp = orphan.get().initComp();
    comp.setIops(ev.iops);
    comp.setFlops(ev.flops);
    comp.setReads(ev.reads);
    comp.setWrites(ev.writes);

    auto &writesRange = ev.uniqueWriteAddrs.get();
    auto numWriteRanges = writesRange.size();
    auto writeAddrBuilder = comp.initWriteAddrs(numWriteRanges);
    size_t i = 0;
    for (auto &p : writesRange)
    {
        auto rangeBuilder = writeAddrBuilder[i++];
        rangeBuilder.setStart(p.first);
        rangeBuilder.setEnd(p.second);
    }

    auto &readsRange = ev.uniqueWriteAddrs.get();
    auto numReadRanges = readsRange.size();
    auto readAddrBuilder = comp.initReadAddrs(numReadRanges);
    size_t j = 0;
    for (auto &p : readsRange)
    {
        auto rangeBuilder = readAddrBuilder[j++];
        rangeBuilder.setStart(p.first);
        rangeBuilder.setEnd(p.second);
    }

    orphans.emplace_back(std::move(orphan));
    flushOrphansOnMaxEvents();
}


auto CapnLoggerCompressed::flush(const STCommEventCompressed& ev, EID eid, TID tid) -> void
{
    (void)eid;
    (void)tid;

    auto orphan = orphanage->getOrphanage().newOrphan<Event>();
    auto commEdgesBuilder = orphan.get().initComm().initEdges(ev.comms.size());;
    for (size_t i=0; i<ev.comms.size(); ++i)
    {
        auto &edge = ev.comms[i];
        commEdgesBuilder[i].setProducerThread(std::get<0>(edge));
        commEdgesBuilder[i].setProducerEvent(std::get<1>(edge));

        auto &ranges = std::get<2>(edge).get();
        auto edgesBuilder = commEdgesBuilder[i].initAddrs(ranges.size());
        size_t j = 0;
        for (auto &p : ranges)
        {
            auto rangeBuilder = edgesBuilder[j++];
            rangeBuilder.setStart(p.first);
            rangeBuilder.setEnd(p.second);
        }
    }

    orphans.emplace_back(std::move(orphan));
    flushOrphansOnMaxEvents();
}


auto CapnLoggerCompressed::flush(unsigned char syncType, Addr syncAddr, EID eid, TID tid) -> void
{
    (void)eid;
    (void)tid;

    flushSyncEvent<Event>(syncType, syncAddr, orphanage, orphans);
    flushOrphansOnMaxEvents();
}


auto CapnLoggerCompressed::instrMarker(int limit) -> void
{
    flushInstrMarker<Event>(limit, orphanage, orphans);
    flushOrphansOnMaxEvents();
}


auto CapnLoggerCompressed::flushOrphansOnMaxEvents() -> void
{
    assert(events <= maxEventsPerMessage);
    if (++events == maxEventsPerMessage)
    {
        flushOrphansAsync();
        events = 0;
    }
}


auto CapnLoggerCompressed::flushOrphansNow() -> void
{
    assert(events <= maxEventsPerMessage);
    if (events > 0)
    {
        flushOrphansAsync();
        doneCopying.get(); // blocking flush
        events = 0;
    }
}


auto CapnLoggerCompressed::flushOrphansAsync() -> void
{
    /* asynchronously copy orphans and flush */
    assert(doneCopying.valid());
    doneCopying.get();
    doneCopying = std::async(std::launch::async,
                             flushOrphans<EventStream, OrphanagePtr, OrphanList>,
                             std::move(orphanage), std::move(orphans), fz);
    /* start a new orphanage */
    orphans.clear();
    orphanage = std::make_unique<::capnp::MallocMessageBuilder>();
}


//-----------------------------------------------------------------------------
/** Single read/write per event **/
CapnLoggerUncompressed::CapnLoggerUncompressed(TID tid, std::string outputPath)
{
    assert(tid >= 1);

    /* initialize orphanage */
    orphanage = std::make_unique<::capnp::MallocMessageBuilder>();

    /* nothing being copied yet */
    doneCopying = std::async([]{return true;});

    auto filePath = (outputPath + "/sigil.events.out-" + std::to_string(tid) +
                     ".uncompressed.capn.bin.gz");
    fz = gzopen(filePath.c_str(), "wb");
    if (fz == NULL)
        fatal(std::string("opening gzfile: ") + strerror(errno));
}


CapnLoggerUncompressed::~CapnLoggerUncompressed()
{
    flushOrphansNow();
    int ret = gzclose(fz);
    if (ret != Z_OK)
        fatal(std::string("closing gzfile: ") + strerror(errno));
}


auto CapnLoggerUncompressed::flush(StatCounter iops, StatCounter flops,
                                   Event::MemType type, Addr start, Addr end,
                                   EID eid, TID tid) -> void
{
    (void)eid;
    (void)tid;

    auto orphan = orphanage->getOrphanage().newOrphan<Event>();
    auto compBuilder = orphan.get().initComp();
    compBuilder.setIops(iops);
    compBuilder.setFlops(flops);
    compBuilder.setMem(type);
    compBuilder.setStartAddr(start);
    compBuilder.setEndAddr(end);

    orphans.emplace_back(std::move(orphan));
    flushOrphansOnMaxEvents();
}


auto CapnLoggerUncompressed::flush(EID producerEID, TID producerTID, Addr start, Addr end,
                                   EID eid, TID tid) -> void
{
    (void)eid;
    (void)tid;

    auto orphan = orphanage->getOrphanage().newOrphan<Event>();
    auto commBuilder = orphan.get().initComm();
    commBuilder.setProducerEvent(producerEID);
    commBuilder.setProducerThread(producerTID);
    commBuilder.setStartAddr(start);
    commBuilder.setEndAddr(end);

    orphans.emplace_back(std::move(orphan));
    flushOrphansOnMaxEvents();
}


auto CapnLoggerUncompressed::flush(unsigned char syncType, Addr syncAddr,
                                   EID eid, TID tid) -> void
{
    (void)eid;
    (void)tid;

    flushSyncEvent<Event>(syncType, syncAddr, orphanage, orphans);
    flushOrphansOnMaxEvents();
}


auto CapnLoggerUncompressed::instrMarker(int limit) -> void
{
    flushInstrMarker<Event>(limit, orphanage, orphans);
    flushOrphansOnMaxEvents();
}


auto CapnLoggerUncompressed::flushOrphansOnMaxEvents() -> void
{
    assert(events <= maxEventsPerMessage);
    if (++events == maxEventsPerMessage)
    {
        flushOrphansAsync();
        events = 0;
    }
}


auto CapnLoggerUncompressed::flushOrphansNow() -> void
{
    assert(events <= maxEventsPerMessage);
    if (events > 0)
    {
        flushOrphansAsync();
        doneCopying.get(); // blocking flush
        events = 0;
    }
}


auto CapnLoggerUncompressed::flushOrphansAsync() -> void
{
    /* asynchronously copy orphans and flush */
    assert(doneCopying.valid());
    doneCopying.get();
    doneCopying = std::async(std::launch::async,
                             flushOrphans<EventStream, OrphanagePtr, OrphanList>,
                             std::move(orphanage), std::move(orphans), fz);

    /* start a new orphanage */
    orphans.clear();
    orphanage = std::make_unique<::capnp::MallocMessageBuilder>();
}

}; //end namespace STGen

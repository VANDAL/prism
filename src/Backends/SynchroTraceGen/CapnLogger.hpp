#ifndef STGEN_CAPNLOGGER_H
#define STGEN_CAPNLOGGER_H

#include "STEvent.hpp"
#include "STTypes.hpp"
#include "Sigil2/SigiLog.hpp"
#include "STEventTrace.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <fcntl.h>

using SigiLog::fatal;
namespace STGen
{

/* Uses CapnProto library (https://capnproto.org)
 * to serialize the event stream to a binary representation.
 * Binary serialization schemes may provide faster parsing
 * of the event stream and better compression */
class CapnLogger
{
    static constexpr unsigned eventsPerMessage = 100000;
  public:
    CapnLogger(const CapnLogger &other) = delete;

    CapnLogger(TID tid, std::string outputPath)
    {
        assert(tid >= 1);

        orphanage = std::make_shared<::capnp::MallocMessageBuilder>();

        auto filePath = outputPath + "/sigil.events.out-" + std::to_string(tid) + ".capn.bin";
        fd = open(filePath.c_str(), O_CREAT | O_TRUNC | O_WRONLY,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        if (fd < 0)
            fatal("Could not open file " + outputPath + " -- " + strerror(errno));
    }

    ~CapnLogger()
    {
        if (events > 0)
        {
            flushOrphans();
            events = 0;
        }

        close(fd);
    }

    /* Log a SynchroTrace aggregate Compute Event */
    auto flush(const STCompEvent& ev, const EID eid, const TID tid) -> void
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

        if (++events == eventsPerMessage)
        {
            flushOrphans();
            events = 0;
        }
    }

    /* Log a SynchroTrace Communication Event */
    auto flush(const STCommEvent& ev, const EID eid, const TID tid) -> void
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

        if (++events == eventsPerMessage)
        {
            flushOrphans();
            events = 0;
        }
    }

    /* Log a SynchroTrace Synchronization Event */
    auto flush(const unsigned char syncType, const Addr syncAddr,
               const EID eid, const TID tid) -> void
    {
        (void)eid;
        (void)tid;
        
        auto orphan = orphanage->getOrphanage().newOrphan<Event>();
        auto syncBuilder = orphan.get().initSync();

        /* translate type to CapnProto enum */
        switch (syncType)
        {
        case 1:
            syncBuilder.setType(::Event::SyncType::LOCK);
            break;
        case 2:
            syncBuilder.setType(::Event::SyncType::UNLOCK);
            break;
        case 3:
            syncBuilder.setType(::Event::SyncType::SPAWN);
            break;
        case 4:
            syncBuilder.setType(::Event::SyncType::JOIN);
            break;
        case 5:
            syncBuilder.setType(::Event::SyncType::BARRIER);
            break;
        case 6:
            syncBuilder.setType(::Event::SyncType::COND_WAIT);
            break;
        case 7:
            syncBuilder.setType(::Event::SyncType::COND_SIGNAL);
            break;
        case 8:
            syncBuilder.setType(::Event::SyncType::COND_BROADCAST);
            break;
        case 9:
            syncBuilder.setType(::Event::SyncType::SPIN_LOCK);
            break;
        case 10:
            syncBuilder.setType(::Event::SyncType::SPIN_UNLOCK);
            break;
        default:
            fatal("capnlogger encountered unhandled sync event");
        }

        syncBuilder.setId(syncAddr);

        orphans.emplace_back(std::move(orphan));

        if (++events == eventsPerMessage)
        {
            flushOrphans();
            events = 0;
        }
    }

    auto flushOrphans() -> void
    {
        ::capnp::MallocMessageBuilder message;
        auto eventStreamBuilder = message.initRoot<EventStream>();
        auto eventsBuilder = eventStreamBuilder.initEvents(events);

        for (unsigned i=0; i<events; ++i)
        {
            auto reader = orphans[i].getReader();
            eventsBuilder.setWithCaveats(i, reader);
        }
        //    eventsBuilder.adoptWithCaveats(i, std::move(orphans[i]));

        orphans.clear();
        ::capnp::writePackedMessageToFd(fd, message);

        orphanage = std::make_shared<::capnp::MallocMessageBuilder>();
    }

  private:
    std::shared_ptr<::capnp::MallocMessageBuilder> orphanage;
    std::vector<::capnp::Orphan<Event>> orphans;
    int fd{-1};
    unsigned events{0};
};

}; //end namespace STGen

#endif

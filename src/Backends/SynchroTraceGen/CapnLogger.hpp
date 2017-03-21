#ifndef STGEN_CAPNLOGGER_H
#define STGEN_CAPNLOGGER_H

#include "STLogger.hpp"
#include "Sigil2/SigiLog.hpp"
#include "STEventTraceCompressed.capnp.h"
#include "STEventTraceUncompressed.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <zlib.h>
#include <future>

/* Uses CapnProto library (https://capnproto.org)
 * to serialize the event stream to a binary representation.
 * Binary serialization schemes may provide faster parsing
 * of the event stream and better compression */

using SigiLog::fatal;

namespace STGen
{

class CapnLoggerCompressed : public STLoggerCompressed
{
    using EventStream = EventStreamCompressed;
    using Event = EventStream::Event;
    using OrphanagePtr = std::unique_ptr<::capnp::MallocMessageBuilder>;
    using OrphanList = std::vector<::capnp::Orphan<Event>>;
  public:
    CapnLoggerCompressed(TID tid, std::string outputPath);
    CapnLoggerCompressed(const CapnLoggerCompressed &other) = delete;
    ~CapnLoggerCompressed() override final;

    auto flush(const STCompEventCompressed& ev, EID eid, TID tid) -> void override final;
    auto flush(const STCommEventCompressed& ev, EID eid, TID tid) -> void override final;
    auto flush(unsigned char syncType, Addr syncAddr, EID eid, TID tid) -> void override final;
    auto instrMarker(int limit) -> void override final;

  private:
    auto flushOrphansOnMaxEvents() -> void;
    auto flushOrphansNow() -> void;
    auto flushOrphansAsync() -> void;

    static constexpr unsigned maxEventsPerMessage = 100000;

    /* use an orphanage because we don't know the event count ahead of time */
    OrphanagePtr orphanage;
    OrphanList orphans;
    gzFile fz;
    unsigned events{0};

    /* Use as a barrier to ensure one capnproto
     * message gets copied at a time */
    std::future<bool> doneCopying;
};


class CapnLoggerUncompressed : public STLoggerUncompressed
{
    using EventStream = EventStreamUncompressed;
    using Event = EventStream::Event;
    using OrphanagePtr = std::unique_ptr<::capnp::MallocMessageBuilder>;
    using OrphanList = std::vector<::capnp::Orphan<Event>>;
  public:
    CapnLoggerUncompressed(TID tid, std::string outputPath);
    CapnLoggerUncompressed(const CapnLoggerUncompressed &other) = delete;
    ~CapnLoggerUncompressed() override final;

    auto flush(StatCounter iops, StatCounter flops,
               Event::MemType type, Addr start, Addr end,
               EID eid, TID tid) -> void override final;
    auto flush(EID producerEID, TID producerTID, Addr start, Addr end,
               EID eid, TID tid) -> void override final;
    auto flush(unsigned char syncType, Addr syncAddr, EID eid, TID tid) -> void override final;
    auto instrMarker(int limit) -> void override final;

  private:
    auto flushOrphansOnMaxEvents() -> void;
    auto flushOrphansNow() -> void;
    auto flushOrphansAsync() -> void;

    static constexpr unsigned maxEventsPerMessage = 500000;

    /* use an orphanage because we don't know the event count ahead of time */
    OrphanagePtr orphanage;
    OrphanList orphans;
    gzFile fz;
    unsigned events{0};

    /* Use as a barrier to ensure one capnproto
     * message gets copied at a time */
    std::future<bool> doneCopying;
};

}; //end namespace STGen

#endif

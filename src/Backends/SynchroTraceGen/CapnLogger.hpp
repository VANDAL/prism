#ifndef STGEN_CAPNLOGGER_H
#define STGEN_CAPNLOGGER_H

#include "STLogger.hpp"
#include "Sigil2/SigiLog.hpp"
#include "STEventTrace.capnp.h"
#include <future>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <zlib.h>

using SigiLog::fatal;

namespace STGen
{

class CapnLogger : public STLogger
{
    /* Uses CapnProto library (https://capnproto.org)
     * to serialize the event stream to a binary representation.
     * Binary serialization schemes may provide faster parsing
     * of the event stream and better compression */
  public:
    CapnLogger(TID tid, std::string outputPath);
    CapnLogger(const CapnLogger &other) = delete;
    ~CapnLogger() override final;

    auto flush(const STCompEvent& ev, EID eid, TID tid) -> void override final;
    auto flush(const STCommEvent& ev, EID eid, TID tid) -> void override final;
    auto flush(unsigned char syncType, Addr syncAddr, EID eid, TID tid) -> void override final;
    auto instrMarker(int limit) -> void override final;

  private:
    auto flushOrphansOnMaxEvents() -> void;
    auto flushOrphansNow() -> void;
    auto flushOrphansAsync() -> void;
    auto flushOrphans(std::vector<::capnp::Orphan<Event>> flushedOrphans,
                      std::unique_ptr<::capnp::MallocMessageBuilder> flushedOrphanage) -> bool;

    static constexpr unsigned maxEventsPerMessage = 100000;

    /* use an orphanage because we don't know the event count ahead of time */
    std::unique_ptr<::capnp::MallocMessageBuilder> orphanage;
    std::vector<::capnp::Orphan<Event>> orphans;
    gzFile fz;
    unsigned events{0};

    /* Use as a barrier to ensure one capnproto
     * message gets copied at a time */
    std::future<bool> doneCopying;
};

}; //end namespace STGen

#endif

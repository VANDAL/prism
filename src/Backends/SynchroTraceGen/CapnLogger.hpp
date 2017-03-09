#ifndef STGEN_CAPNLOGGER_H
#define STGEN_CAPNLOGGER_H

#include "STLogger.hpp"
#include "Sigil2/SigiLog.hpp"
#include "STEventTrace.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <zlib.h>
#include <fcntl.h>

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
    ~CapnLogger();

    auto flush(const STCompEvent& ev, EID eid, TID tid) -> void override final;
    auto flush(const STCommEvent& ev, EID eid, TID tid) -> void override final;
    auto flush(unsigned char syncType, Addr syncAddr, EID eid, TID tid) -> void override final;
    auto instrMarker(int limit) -> void override final;

  private:
    auto flushOrphans() -> void;

    static constexpr unsigned eventsPerMessage = 100000;
    std::shared_ptr<::capnp::MallocMessageBuilder> orphanage;
    std::vector<::capnp::Orphan<Event>> orphans;
    gzFile fz;
    unsigned events{0};
};

}; //end namespace STGen

#endif

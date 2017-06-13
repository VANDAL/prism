#ifndef STGEN_NULL_LOGGER_H
#define STGEN_NULL_LOGGER_H

#include "STLogger.hpp"

namespace STGen
{

class NullLogger : public STLoggerCompressed, public STLoggerUncompressed
{
    /* for testing */
  public:
    NullLogger(TID tid, std::string outputPath)
    {
        assert(tid >= 1);
        (void)tid;
        (void)outputPath;
    }

    auto flush(const STCompEventCompressed& ev, EID eid, TID tid) -> void override final
    {
        (void)ev;
        (void)eid;
        (void)tid;
    }

    auto flush(const STCommEventCompressed& ev, EID eid, TID tid) -> void override final
    {
        (void)ev;
        (void)eid;
        (void)tid;
    }

    auto flush(StatCounter iops, StatCounter flops,
               STCompEventUncompressed::MemType type, Addr start, Addr end,
               EID eid, TID tid) -> void override final
    {
        (void)iops;
        (void)flops;
        (void)type;
        (void)start;
        (void)end;
        (void)eid;
        (void)tid;
    }

    auto flush(EID producerEID, TID producerTID, Addr start, Addr end,
               EID eid, TID tid) -> void override final
    {
        (void)producerEID;
        (void)producerTID;
        (void)start;
        (void)end;
        (void)eid;
        (void)tid;
    }

    auto flush(unsigned char syncType, unsigned numArgs, Addr *syncArgs,
               EID eid, TID tid) -> void override final
    {
        (void)syncType;
        (void)numArgs;
        (void)syncArgs;
        (void)eid;
        (void)tid;
    }

    auto instrMarker(int limit) -> void override final
    {
        (void)limit;
    }
};

}; //end namespace STGen

#endif

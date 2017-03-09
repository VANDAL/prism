#ifndef STGEN_NULL_LOGGER_H
#define STGEN_NULL_LOGGER_H

#include "STLogger.hpp"

namespace STGen
{

/* for testing */
class NullLogger : public STLogger
{
  public:
    NullLogger(TID tid, std::string outputPath)
    {
        assert(tid >= 1);
        (void)tid;
        (void)outputPath;
    }

    auto flush(const STCompEvent& ev, EID eid, TID tid) -> void override final
    {
        (void)ev;
        (void)eid;
        (void)tid;
    }

    auto flush(const STCommEvent& ev, EID eid, TID tid) -> void override final
    {
        (void)ev;
        (void)eid;
        (void)tid;
    }

    auto flush(unsigned char syncType, Addr syncAddr,
               EID eid, TID tid) -> void override final
    {
        (void)syncType;
        (void)syncAddr;
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

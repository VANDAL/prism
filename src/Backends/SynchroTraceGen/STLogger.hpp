#ifndef STGEN_LOGGER_H
#define STGEN_LOGGER_H

#include "STTypes.hpp"
#include "STEvent.hpp"

/*****************************************************************************
 * Two abstract loggers to handle two fundamentally different event data:
 * - 'compressed' events vs un'compressed' events
 *****************************************************************************/


namespace STGen
{

class STLoggerCompressed
{
  public:
    virtual ~STLoggerCompressed() {}

    virtual auto flush(const STCompEventCompressed& ev, EID eid, TID tid) -> void = 0;
    /* Log a SynchroTrace aggregate Compute Event */

    virtual auto flush(const STCommEventCompressed& ev, EID eid, TID tid) -> void = 0;
    /* Log a SynchroTrace Communication Event */

    virtual auto flush(unsigned char syncType, unsigned numArgs, Addr *syncArgs,
                       EID eid, TID tid) -> void = 0;
    /* Log a SynchroTrace Synchronization Event */

    virtual auto instrMarker(int limit) -> void = 0;
    /* Place a marker in the trace after 'limit' instructions */
};

class STLoggerUncompressed
{
  public:
    virtual ~STLoggerUncompressed() {}

    virtual auto flush(StatCounter iops, StatCounter flops,
                       STCompEventUncompressed::MemType type, Addr start, Addr end,
                       EID eid, TID tid) -> void = 0;
    /* Log a SynchroTrace aggregate Compute Event */

    virtual auto flush(EID producerEID, TID producerTID, Addr start, Addr end,
                       EID eid, TID tid) -> void = 0;
    /* Log a SynchroTrace Communication Event */

    virtual auto flush(unsigned char syncType, unsigned numArgs, Addr *syncArgs,
                       EID eid, TID tid) -> void = 0;
    /* Log a SynchroTrace Synchronization Event */

    virtual auto instrMarker(int limit) -> void = 0;
    /* Place a marker in the trace after 'limit' instructions */
};

}; //end namespace STGen

#endif

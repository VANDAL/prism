#ifndef STGEN_LOGGER_H
#define STGEN_LOGGER_H

#include "STTypes.hpp"
#include "STEvent.hpp"

namespace STGen
{

class STLogger
{
  public:
    /* Log a SynchroTrace aggregate Compute Event */
    virtual auto flush(const STCompEvent& ev, EID eid, TID tid) -> void = 0;

    /* Log a SynchroTrace Communication Event */
    virtual auto flush(const STCommEvent& ev, EID eid, TID tid) -> void = 0; 

    /* Log a SynchroTrace Synchronization Event */
    virtual auto flush(unsigned char syncType, Addr syncAddr, EID eid, TID tid) -> void = 0;

    /* Place a marker in the trace after 'limit' instructions */
    virtual auto instrMarker(int limit) -> void = 0;
};

}; //end namespace STGen

#endif

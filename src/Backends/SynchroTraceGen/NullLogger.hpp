#ifndef STGEN_NULL_LOGGER_H
#define STGEN_NULL_LOGGER_H

#include "STEvent.hpp"
#include "STTypes.hpp"

namespace STGen
{

/* for testing */
class NullLogger
{
  public:
    NullLogger(TID tid, std::string outputPath) { assert(tid >= 1); }
    auto flush(const STCompEvent& ev, const EID eid, const TID tid) -> void {}
    auto flush(const STCommEvent& ev, const EID eid, const TID tid) -> void {}
    auto flush(const unsigned char syncType, const Addr syncAddr,
               const EID eid, const TID tid) -> void {}

};

}; //end namespace STGen

#endif

#ifndef STGEN_LOGGER_H
#define STGEN_LOGGER_H

#include "STTypes.hpp"
#include "STEvent.hpp"

namespace STGen
{

/* Two abstract loggers to handle two fundamentally different event data:
 * - 'compressed' events vs un'compressed' events */

class STLoggerCompressed
{
  public:
    virtual ~STLoggerCompressed() {}

    /* Log a SynchroTrace aggregate Compute Event */
    virtual auto flush(const STCompEventCompressed& ev, EID eid, TID tid) -> void = 0;

    /* Log a SynchroTrace Communication Event */
    virtual auto flush(const STCommEventCompressed& ev, EID eid, TID tid) -> void = 0; 

    /* Log a SynchroTrace Synchronization Event */
    virtual auto flush(unsigned char syncType, Addr syncAddr, EID eid, TID tid) -> void = 0;

    /* Place a marker in the trace after 'limit' instructions */
    virtual auto instrMarker(int limit) -> void = 0;
};

class STLoggerUncompressed
{
  public:
    virtual ~STLoggerUncompressed() {}

    /* Log a SynchroTrace aggregate Compute Event */
    virtual auto flush(StatCounter iops, StatCounter flops,
                       STCompEventUncompressed::MemType type, Addr start, Addr end,
                       EID eid, TID tid) -> void = 0;

    /* Log a SynchroTrace Communication Event */
    virtual auto flush(EID producerEID, TID producerTID, Addr start, Addr end,
                       EID eid, TID tid) -> void = 0; 

    /* Log a SynchroTrace Synchronization Event */
    virtual auto flush(unsigned char syncType, Addr syncAddr, EID eid, TID tid) -> void = 0;

    /* Place a marker in the trace after 'limit' instructions */
    virtual auto instrMarker(int limit) -> void = 0;
};

/* classes that inherit these classes should take a TID and output path */
template <typename LoggerType>
using LogGenerator = std::function<std::unique_ptr<LoggerType>(TID, std::string)>;

template <class LoggerType>
auto LogGeneratorFactory(TID tid, std::string outputPath) -> std::unique_ptr<LoggerType>
{
    return std::unique_ptr<LoggerType>(new LoggerType{tid, outputPath});
}

}; //end namespace STGen

#endif

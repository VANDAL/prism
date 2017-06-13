#ifndef STGEN_THREAD_CONTEXT_H
#define STGEN_THREAD_CONTEXT_H

#include "STEvent.hpp"
#include "TextLogger.hpp"
#include "STTypes.hpp"

/* DynamoRIO sometimes reports very high addresses.
 * For now, allow these addresses until we figure
 * out what to do with them */
#define ALLOW_ADDRESS_OVERFLOW 1
#include "STShadowMemory.hpp"

/* XXX overflow builtin not in GCC <5.
 * This overflow check should only be used for
 * variables which increment by 1 each time. */
#if __GNUC__ >= 5
#define INCR_EID_OVERFLOW(var) __builtin_add_overflow(var, 1, &var)
#else
#define INCR_EID_OVERFLOW(var) (var == UINT_MAX ? true : (var += 1) && false)
#endif


namespace STGen
{

class ThreadContext
{
    /* SynchroTraceGen makes use of 3 SynchroTrace events,
     * i.e. Computation, Communication, and Synchronization.
     *
     * An event aggregates metadata and is eventually flushed to a log.
     * Synchronization events are immediately flushed,
     * so no event state needs to be tracked */
  public:
    virtual ~ThreadContext() {}
    virtual auto getStats() const -> PerThreadStats = 0;
    virtual auto onIop() -> void = 0;
    virtual auto onFlop() -> void = 0;
    virtual auto onRead(Addr start, Addr bytes) -> void = 0;
    virtual auto onWrite(Addr start, Addr bytes) -> void = 0;
    virtual auto onSync(unsigned char syncType, unsigned numArgs, Addr *syncArgs) -> void = 0;
    /* sync functions support one or two arguments;
     * the second argument is optional */

    virtual auto onInstr() -> void = 0;
    virtual auto flushAll() -> void = 0;

  protected:
    static STShadowMemory shadow; // Shadow memory is shared amongst all threads
};


class ThreadContextCompressed : public ThreadContext
{
    using LogPtr = std::unique_ptr<STLoggerCompressed>;
  public:
    ThreadContextCompressed(TID tid, unsigned primsPerStCompEv,
                            std::string outputPath, std::string loggerType);
    ~ThreadContextCompressed();

    auto getStats() const -> PerThreadStats override final;
    auto onIop() -> void override final;
    auto onFlop() -> void override final;
    auto onRead(Addr start, Addr bytes) -> void override final;
    auto onWrite(Addr start, Addr bytes) -> void override final;
    auto onSync(unsigned char syncType, unsigned numArgs, Addr *syncArgs) -> void override final;
    auto onInstr() -> void override final;
    auto flushAll() -> void override final;

  private:
    auto checkCompFlushLimit() -> void;
    auto compFlushIfActive() -> void;
    auto commFlushIfActive() -> void;
    static auto getLogger(TID tid, std::string outputPath, std::string loggerType) -> LogPtr;

    STCompEventCompressed stComp;
    STCommEventCompressed stComm;

    TID tid;
    unsigned primsPerStCompEv;
    /* compression level of events */

    StatCounter events{0};
    PerThreadStats stats;
    /* track statistics */

    LogPtr logger;
};


class ThreadContextUncompressed : public ThreadContext
{
    using LogPtr = std::unique_ptr<STLoggerUncompressed>;
  public:
    ThreadContextUncompressed(TID tid, unsigned primsPerStCompEv,
                              std::string outputPath, std::string loggerType);
    ~ThreadContextUncompressed();

    auto getStats() const -> PerThreadStats override final;
    auto onIop() -> void override final;
    auto onFlop() -> void override final;
    auto onRead(Addr start, Addr bytes) -> void override final;
    auto onWrite(Addr start, Addr bytes) -> void override final;
    auto onSync(unsigned char syncType, unsigned numArgs, Addr *syncArgs) -> void override final;
    auto onInstr() -> void override final;
    auto flushAll() -> void override final;

  private:
    auto compFlushIfActive() -> void;
    auto compFlush(STCompEventUncompressed::MemType type, Addr start, Addr end) -> void;
    auto commFlush(EID producerEID, TID producerTID, Addr start, Addr end) -> void;
    static auto getLogger(TID tid, std::string outputPath, std::string loggerType) -> LogPtr;

    STCompEventUncompressed stComp;

    TID tid;
    unsigned primsPerStCompEv;
    /* compression level of events */

    StatCounter events{0};
    PerThreadStats stats;
    /* track statistics */

    LogPtr logger;
};

}; //end namespace STGen

#endif

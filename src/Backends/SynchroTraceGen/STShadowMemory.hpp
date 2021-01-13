#ifndef STGEN_SHADOWMEMORY_H
#define STGEN_SHADOWMEMORY_H

#include "ShadowMemory.hpp"
#include "STTypes.hpp"

#include <cstdint>
#include <bitset>


namespace STGen
{

constexpr TID SO_UNDEF = -1;
constexpr TID MAX_THREADS = 128;
static_assert((MAX_THREADS > 0) && !(MAX_THREADS & (MAX_THREADS-1)),
              "MAX_THREADS must be a power of 2");

class STShadowMemory
{
    /* In SynchroTraceGen, 'shadow state' takes the form of
     * the most recent thread to {read from, write to} an address.  */
  public:
    auto updateWriter(Addr addr, uint32_t bytes, TID tid, EID eid) -> void;
    auto updateReader(Addr addr, uint32_t bytes, TID tid) -> void;
    auto getWriterTID(Addr addr) -> TID;
    auto getWriterEID(Addr addr) -> EID;
    auto isReaderTID(Addr addr, TID tid) -> bool;

    struct ShadowObject
    {
        TID last_writer{SO_UNDEF};
        EID last_writer_event{0};
        /* Last thread/event to read/write to addr */
    
        std::bitset<MAX_THREADS> last_readers;
        /* A bitfield -- each bit represents a thread
         * each address can have multiple readers */
    };

    ShadowMemory<ShadowObject, 38, 20> sm;
    /* ADDR_BITS = 48, PM_BITS = 28 is more appropriate for DynamoRIO */
};


inline auto STShadowMemory::updateWriter(Addr addr, uint32_t bytes, TID tid, EID eid) -> void
{
    assert(tid < MAX_THREADS);
    for (uint32_t i = 0; i < bytes; ++i)
    {
        ShadowObject &so = sm[addr + i];
        so.last_writer = tid;
        so.last_writer_event = eid;
        so.last_readers.reset();
    }
}


inline auto STShadowMemory::updateReader(Addr addr, uint32_t bytes, TID tid) -> void
{
    assert(tid < MAX_THREADS);
    for (uint32_t i = 0; i < bytes; ++i)
    {
        ShadowObject &so = sm[addr + i];
        so.last_readers.set(tid);
    }
}


inline auto STShadowMemory::isReaderTID(Addr addr, TID tid) -> bool
{
    assert(tid < MAX_THREADS);
    ShadowObject &so = sm[addr];
    return so.last_readers.test(tid);
}


inline auto STShadowMemory::getWriterTID(Addr addr) -> TID
{
    return sm[addr].last_writer;
}


inline auto STShadowMemory::getWriterEID(Addr addr) -> EID
{
    return sm[addr].last_writer_event;
}

}; //end namespace STGen

#endif

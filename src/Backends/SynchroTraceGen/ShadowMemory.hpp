#ifndef STGEN_SHADOWMEMORY_H
#define STGEN_SHADOWMEMORY_H

#include "Sigil2/Primitive.h" // Addr type
#include "MemoryPool.h"

#include <cstdint>
#include <vector>
#include <memory>

/* Shadow Memory tracks 'shadow state' for an address.
 *
 * In SynchroTraceGen, 'shadow state' takes the form of
 * the most recent thread to read from/write to an address.
 *
 * For further clarification, please read,
 * "How to Shadow Every Byte of Memory Used by a Program"
 * by Nicholas Nethercote and Julian Seward
 */

namespace STGen
{

/* XXX Thread ID (TID) and Event ID (EID)
 * set to 16-bits and 32-bits for memory usage considerations.
 * Increasing the sizes may be required in the future. */
using TID = int16_t;
using EID = int32_t;
constexpr TID SO_UNDEF = -1;

class ShadowMemory
{
  public:
    /* XXX: setting max address bits ABOVE 63
     * has undefined behavior;
     *
     * XXX: Setting addr/pm bits too large can cause
     * bad_alloc errors */
    ShadowMemory(Addr addr_bits = 38, Addr pm_bits = 16, Addr max_shad_mem_size = 4096/*MB*/);
    ShadowMemory(const ShadowMemory &) = delete;
    ShadowMemory &operator=(const ShadowMemory &) = delete;
    ~ShadowMemory();

    auto updateWriter(Addr addr, ByteCount bytes, TID tid, EID event_id) -> void;
    auto updateReader(Addr addr, ByteCount bytes, TID tid) -> void;
    auto getWriterTID(Addr addr) -> TID;
    auto getWriterEID(Addr addr) -> EID;
    auto isReaderTID(Addr addr, TID tid) -> bool;

    /* Configuration */
    const Addr addr_bits;
    const Addr pm_bits;
    const Addr sm_bits;
    const Addr pm_size;
    const Addr sm_size;

    /* In MB
     * Not an exact count, but good enough to know when
     * shadow memory grows too large, assuming SMs are
     * sufficiently large */
    const Addr max_shad_mem_size;
    Addr curr_shad_mem_size;
    Addr curr_sm_count;
    Addr pm_Mbytes;
    Addr sm_Mbytes;

  private:
    /* Secondary/Primary Maps */
    MemoryPool<TID> pool;
    struct SecondaryMap
    {
        SecondaryMap(size_t sm_size) : last_writers(sm_size, SO_UNDEF)
                                     , last_writers_event(sm_size, SO_UNDEF)
                                     , last_readers(sm_size, {SO_UNDEF}){}

        /* Last thread/event to read/write to addr */
        std::vector<TID> last_writers;
        std::vector<EID> last_writers_event;
        /* A vector of vectors -- each address can have multiple readers */
        std::vector<std::vector<TID>> last_readers;
    };

    /* Primary Map is a sparse vector.
     * Pointers are used because that implementation
     * is more memory efficient for a sparse vector. */
    std::vector<SecondaryMap *> PM;

    /* Utility Functions */
    auto getSMFromAddr(Addr addr) -> SecondaryMap&;
    auto initSM(SecondaryMap *&SM) -> void;
    auto getSMidx(Addr addr) const -> size_t;
    auto getPMidx(Addr addr) const -> size_t;
    auto addSMsize() -> void;
};

}; //end namespace STGen

#endif

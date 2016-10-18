#ifndef SIGILCLASSIC_SHADOWMEMORY_H
#define SIGILCLASSIC_SHADOWMEMORY_H

#include "Sigil2/Primitive.h" // Addr type

#include <cstdint>
#include <vector>
#include <memory>

/* Shadow Memory tracks 'shadow state' for an address.  *
 * In SigilClassic, 'shadow state' takes the form of
 * the most recent 'entity' to read from/write to an address.
 * That 'entity' could be in the form of a context such as
 * a function, basic block, or thread. This implementationo
 * uses functions.
 *
 * For further clarification, please read,
 * "How to Shadow Every Byte of Memory Used by a Program"
 * by Nicholas Nethercote and Julian Seward
 */

namespace SigilClassic
{

/* Function ID (FID) and Event ID (EID) */
using FID = int32_t;
constexpr FID SO_UNDEF{-1};

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

    auto updateWriter(Addr addr, ByteCount bytes, FID fid) -> void;
    auto updateReader(Addr addr, ByteCount bytes, FID fid) -> void;
    auto getWriterFID(Addr addr) -> FID;
    auto isReaderFID(Addr addr, FID fid) -> bool;

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
    struct ShadowMemoryImpl
    {
        struct ShadowObject
        {
            /* Last function to write to addr */
            FID last_writer{SO_UNDEF};

            /* Last function to read addr */
            FID last_reader{SO_UNDEF};
        };

        using SecondaryMap = std::vector<ShadowObject>;
        using SecondaryMapPtr = std::unique_ptr<SecondaryMap>;
        struct PrimaryMap
        {
            /* for accessing shadow memory configuration */
            PrimaryMap(ShadowMemory &sm) : sm(sm), primary_map_(sm.pm_size) {}
            ShadowMemory &sm;

            /* Primary Map is a sparse vector.
             * Pointers are used because that implementation
             * is more memory efficient for a sparse vector. */
            std::vector<SecondaryMapPtr> primary_map_;

            auto operator[](Addr pm_offset) -> SecondaryMap&;
        } PM;

        ShadowMemoryImpl(ShadowMemory &sm) : PM(sm), sm(sm) {}
        ShadowMemory &sm;

        auto operator[](Addr addr) -> ShadowObject&;
    };

    ShadowMemoryImpl shadow_objects;
    using ShadowObject = ShadowMemoryImpl::ShadowObject;

    /* Utility Functions */
    auto addSMsize() -> void;
};

}; //end namespace SigilClassic

#endif

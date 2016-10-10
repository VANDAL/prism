#include "Sigil2/SigiLog.hpp"
#include "ShadowMemory.hpp"

namespace STGen
{

auto ShadowMemory::updateWriter(Addr addr, ByteCount bytes, TID tid, EID eid) -> void
{
    assert(tid < MAX_THREADS);
    for (ByteCount i = 0; i < bytes; ++i)
    {
        ShadowObject &so = shadow_objects[addr + i];

        so.last_writer = tid;
        so.last_writer_event = eid;

        /* Reset readers on new write */
        std::fill(so.last_readers, (so.last_readers + LAST_READER_SECTION_COUNT), 0);
    }
}


auto inline TID_bit_value(TID tid) -> reader_thr_t
{
    return 1 << (tid & (LAST_READER_SECTION_SIZE - 1));
}

auto ShadowMemory::updateReader(Addr addr, ByteCount bytes, TID tid) -> void
{
    assert(tid < MAX_THREADS);
    for (ByteCount i = 0; i < bytes; ++i)
    {
        ShadowObject &so = shadow_objects[addr + i];

        /* set thread bit in last reader section */
        auto section = tid / LAST_READER_SECTION_SIZE;
        reader_thr_t &last_reader_section = so.last_readers[section];
        last_reader_section |= TID_bit_value(tid);
    }
}


auto ShadowMemory::isReaderTID(Addr addr, TID tid) -> bool
{
    assert(tid < MAX_THREADS);
    ShadowObject &so = shadow_objects[addr];

    auto section = tid / LAST_READER_SECTION_SIZE;
    reader_thr_t last_reader_section = so.last_readers[section];

    return TID_bit_value(tid) & last_reader_section;
}


auto ShadowMemory::getWriterTID(Addr addr) -> TID
{
    return shadow_objects[addr].last_writer;
}


auto ShadowMemory::getWriterEID(Addr addr) -> EID
{
    return shadow_objects[addr].last_writer_event;
}


ShadowMemory::ShadowMemory(Addr addr_bits, Addr pm_bits, Addr max_shad_mem_size)
    : addr_bits(addr_bits)
    , pm_bits(pm_bits)
    , sm_bits(addr_bits - pm_bits)
    , pm_size(1ULL << pm_bits)
    , sm_size(1ULL << sm_bits)
    , max_shad_mem_size(max_shad_mem_size)
    , curr_shad_mem_size(0)
    , curr_sm_count(0)
    , shadow_objects(*this) /* initialize primary map */
{
    assert(addr_bits > 0);
    assert(pm_bits > 0);

    pm_Mbytes = pm_size * sizeof(ShadowMemoryImpl::SecondaryMapPtr) / (1 << 20);
    sm_Mbytes = sm_size * (sizeof(TID) * 2 + sizeof(EID)) / (1 << 20);

    if (sm_Mbytes < 1)
    {
        SigiLog::fatal("SM size too small - adjust shadow memory configs in source code");
    }

    curr_shad_mem_size += pm_Mbytes;
}


/* debug calculations */
ShadowMemory::~ShadowMemory()
{
    size_t pm_theoretical = sizeof(void*)*pm_size;
    size_t pm_actual = shadow_objects.PM.primary_map_.capacity()*sizeof(ShadowMemoryImpl::SecondaryMapPtr);
    SigiLog::debug(std::string("PM theoretical: ").append(std::to_string(pm_theoretical)));
    SigiLog::debug(std::string("PM actual:      ").append(std::to_string(pm_actual)));

    size_t sm_theoretical = (sizeof(TID) * 2 + sizeof(EID))*sm_size;
    SigiLog::debug(std::string("SM theoretical: ").append(std::to_string(sm_theoretical*curr_sm_count)));

    size_t sm_actual = 0;
    for (auto &sm: shadow_objects.PM.primary_map_)
    {
        if (sm != nullptr)
        {
            sm_actual += sizeof(ShadowMemoryImpl::SecondaryMap);
            sm_actual += sm->capacity()*sizeof(ShadowMemoryImpl::ShadowObject);
        }
    }

    SigiLog::debug(std::string("SM actual:      ").append(std::to_string(sm_actual)));
    SigiLog::debug("Secondary Maps used: " + std::to_string(curr_sm_count));
}


///////////////////////////////////////
// Utility Functions
///////////////////////////////////////
inline auto ShadowMemory::addSMsize() -> void
{
    ++curr_sm_count;
    curr_shad_mem_size += sm_Mbytes;

    if (curr_shad_mem_size > max_shad_mem_size)
    {
        SigiLog::fatal("shadow memory size limits exceeded");
    }
}


///////////////////////////////////////
// Implementation
///////////////////////////////////////
namespace { [[noreturn]] auto out_of_range_fatal(Addr addr) -> void; };

inline auto ShadowMemory::ShadowMemoryImpl::operator[](Addr addr) -> ShadowObject&
{
    if ((addr >> sm.addr_bits) > 0)
    {
        out_of_range_fatal(addr);
    }

    /* primary map and secondary map offsets */
    return PM[addr >> sm.sm_bits][addr & ((1ULL << sm.sm_bits) - 1)];
}


inline auto ShadowMemory::ShadowMemoryImpl::PrimaryMap::operator[](Addr pm_offset) -> SecondaryMap&
{
    SecondaryMapPtr &ptr = primary_map_[pm_offset];

    if (ptr == nullptr)
    {
        ptr = SecondaryMapPtr(new SecondaryMap(sm.sm_size));
        sm.addSMsize(); // stat tracking
    }

    return *ptr;
}


namespace
{
[[noreturn]] auto out_of_range_fatal(Addr addr) -> void
{
    char s_addr[32];
    sprintf(s_addr, "0x%lx", addr);

    std::string msg("shadow memory max address limit [");
    msg.append(s_addr).append("]");

    SigiLog::fatal(msg);
}
};

}; //end namespace STGen

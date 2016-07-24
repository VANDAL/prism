#include "Sigil2/SigiLog.hpp"
#include "ShadowMemory.hpp"
#include <sstream> // for printing hex vals

namespace STGen
{

auto ShadowMemory::updateWriter(Addr addr, ByteCount bytes, TID tid, EID eid) -> void
{
    for (ByteCount i = 0; i < bytes; ++i)
    {
        Addr curr_addr = addr + i;
        SecondaryMap &sm = getSMFromAddr(curr_addr);

        sm.last_writers[getSMidx(curr_addr)] = tid;
        sm.last_writers_event[getSMidx(curr_addr)] = eid;

        /* Reset readers on new write */
        sm.last_readers[getSMidx(curr_addr)].assign({SO_UNDEF});
    }
}


auto ShadowMemory::updateReader(Addr addr, ByteCount bytes, TID tid) -> void
{
    for (ByteCount i = 0; i < bytes; ++i)
    {
        Addr curr_addr = addr + i;
        SecondaryMap &sm = getSMFromAddr(curr_addr);
        sm.last_readers[getSMidx(curr_addr)].push_back(tid);
    }
}


auto ShadowMemory::isReaderTID(Addr addr, TID tid) -> bool
{
    for /*each reader*/(TID reader : getSMFromAddr(addr).last_readers[getSMidx(addr)])
    {
        if (reader == tid)
        {
            return true;
        }
    }

    return false;
}


auto ShadowMemory::getWriterTID(Addr addr) -> TID
{
    return getSMFromAddr(addr).last_writers[getSMidx(addr)];
}


auto ShadowMemory::getWriterEID(Addr addr) -> EID
{
    return getSMFromAddr(addr).last_writers_event[getSMidx(addr)];
}


ShadowMemory::ShadowMemory(Addr addr_bits, Addr pm_bits, Addr max_shad_mem_size)
    : addr_bits(addr_bits)
    , pm_bits(pm_bits)
    , sm_bits(addr_bits - pm_bits)
    , pm_size(1ULL << pm_bits)
    , sm_size(1ULL << sm_bits)
    , max_shad_mem_size(max_shad_mem_size)
    , curr_shad_mem_size(0)   /* includes DSM */
    , curr_sm_count(0)   /* excludes DSM */
{
    assert(addr_bits > 0);
    assert(pm_bits > 0);

    pm_Mbytes = pm_size * sizeof(char *) / (1 << 20);
    sm_Mbytes = sm_size * (sizeof(TID) * 2 + sizeof(EID)) / (1 << 20);

    if (sm_Mbytes < 1)
    {
        SigiLog::fatal("SM size too small - adjust shadow memory configs in source code");
    }

    SigiLog::debug("Shadow Memory PM size: " + std::to_string(pm_Mbytes) + " MB");
    SigiLog::debug("Shadow Memory SM size: " + std::to_string(sm_Mbytes) + " MB");

    curr_shad_mem_size += pm_Mbytes;
    curr_shad_mem_size += sm_Mbytes;

    /* initialize primary map */
    PM.assign(pm_size, nullptr);
}


ShadowMemory::~ShadowMemory()
{
    SigiLog::debug("Shadow Memory approximate size: " + std::to_string(curr_shad_mem_size) + " MB");
    SigiLog::debug("Shadow Memory Secondary Maps used: " + std::to_string(curr_sm_count));

    for (auto SM : PM)
    {
        delete SM;
    }
}


///////////////////////////////////////
// Utility Functions
///////////////////////////////////////
inline auto ShadowMemory::getSMFromAddr(Addr addr) -> SecondaryMap&
{
    if /*out of range*/((addr >> addr_bits) > 0)
    {
        char s_addr[32];
        sprintf(s_addr, "0x%lx", addr);

        std::string msg("shadow memory max address limit: ");
        msg.append(s_addr);

        SigiLog::fatal(msg);
    }

    SecondaryMap *&SM = PM[getPMidx(addr)];

    if /*uninitialized*/(SM == nullptr)
    {
        SM = new SecondaryMap(sm_size);
        addSMsize(); // stat tracking
    }

    return *SM;
}


inline auto ShadowMemory::getSMidx(Addr addr) const -> size_t
{
    return addr & ((1ULL << sm_bits) - 1);
}


inline auto ShadowMemory::getPMidx(Addr addr) const -> size_t
{
    return addr >> sm_bits;
}


inline auto ShadowMemory::addSMsize() -> void
{
    ++curr_sm_count;
    curr_shad_mem_size += sm_Mbytes;

    if (curr_shad_mem_size > max_shad_mem_size)
    {
        SigiLog::fatal("shadow memory size limits exceeded");
    }
}

}; //end namespace STGen

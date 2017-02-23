#ifndef SC_SHADOWMEMORY_H
#define SC_SHADOWMEMORY_H

#include "ShadowMemory.hpp"

namespace SigilClassic
{

/* XXX Fhread ID (TID)
 * set to 32-bits for memory usage considerations.
 * Increasing the sizes may be required in the future. */
using UInt = uint32_t;
using Int = int32_t;
using FID = Int;
constexpr FID SO_UNDEF{-1};

class SCShadowMemory
{
  public:
    auto updateWriter(Addr addr, ByteCount bytes, FID fid) -> void;
    auto updateReader(Addr addr, ByteCount bytes, FID fid) -> void;
    auto isReaderFID(Addr addr, FID fid) -> bool;
    auto getWriterFID(Addr addr) -> FID;

    struct ShadowObject
    {
        FID last_writer{SO_UNDEF}; // Last function to write to addr
        FID last_reader{SO_UNDEF}; //Last function to read addr
    };

    ShadowMemory<ShadowObject, 45, 28> sm;
};

inline auto SCShadowMemory::updateWriter(Addr addr, ByteCount bytes, FID fid) -> void
{
    for (ByteCount i = 0; i < bytes; ++i)
    {
        ShadowObject &so = sm[addr + i];
        so.last_writer = fid;
        so.last_reader = SO_UNDEF; // Reset readers on new write
    }
}


inline auto SCShadowMemory::updateReader(Addr addr, ByteCount bytes, FID fid) -> void
{
    for (ByteCount i = 0; i < bytes; ++i)
    {
        ShadowObject &so = sm[addr + i];
        so.last_reader = fid;
    }
}


inline auto SCShadowMemory::isReaderFID(Addr addr, FID fid) -> bool
{
    ShadowObject &so = sm[addr];
    return so.last_reader == fid;
}


inline auto SCShadowMemory::getWriterFID(Addr addr) -> FID
{
    return sm[addr].last_writer;
}
}; //end namespace SigilClassic

#endif

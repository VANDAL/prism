#ifndef SHADOWMEMORY_H
#define SHADOWMEMORY_H

#include "Core/Primitive.h" // PtrVal type
#include "Core/SigiLog.hpp"

#include <limits>
#include <vector>
#include <memory>
#include <stdexcept>

/* Shadow Memory tracks 'shadow state' for an address.
 * For further clarification, please read,
 * "How to Shadow Every Byte of Memory Used by a Program"
 * by Nicholas Nethercote and Julian Seward
 */

using Addr = PtrVal;
using SigiLog::fatal;
using SigiLog::warn;

/* XXX: Setting {addr, pm} bits too large can cause bad_alloc errors */
template <typename SO, unsigned ADDR_BITS = 38, unsigned PM_BITS = 16>
class ShadowMemory
{
    static_assert(ADDR_BITS > 0 && ADDR_BITS < 64, "Invalid address range");
    static_assert(PM_BITS > 0, "Invalid offset for primary map");
    static_assert(sizeof(Addr)*CHAR_BIT >= ADDR_BITS, "Max address is too large for the platform");

  public:
    ShadowMemory()
        : addr_bits(ADDR_BITS)
        , pm_bits(PM_BITS)
        , sm_bits(addr_bits - pm_bits)
        , pm_size(1ULL << pm_bits)
        , sm_size(1ULL << sm_bits)
        , pm(pm_size)
    {}
    ShadowMemory(const ShadowMemory &) = delete;
    ShadowMemory &operator=(const ShadowMemory &) = delete;

    /* Configuration */
    const Addr addr_bits;
    const Addr pm_bits;
    const Addr sm_bits;
    const Addr pm_size;
    const Addr sm_size;

    /* Implementation */
    using SecondaryMap = std::vector<SO>;
    using PrimaryMap = std::vector<std::unique_ptr<SecondaryMap>>;

    auto operator[](Addr addr) -> SO&
    {
        if ((addr >> addr_bits) == 0)
        {
            auto &ptr = pm[addr >> sm_bits]; /* PM offset */
            if (ptr == nullptr)
                ptr = std::make_unique<SecondaryMap>(sm_size);

            return (*ptr)[addr & ((1ULL << sm_bits) - 1)]; /* SM offset */
        }
        else
        {
            char s_addr[32];
            sprintf(s_addr, "0x%lx", addr);
            auto msg = std::string("shadow memory max address limit [").append(s_addr).append("]");
#ifdef ALLOW_ADDRESS_OVERFLOW
            /* let the caller figure out what it wants to do */
            throw std::out_of_range(msg);
#else
            fatal(msg);
#endif
        }
    }

  private:
    PrimaryMap pm;

};

#endif

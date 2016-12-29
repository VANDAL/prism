#ifndef SHADOWMEMORY_H
#define SHADOWMEMORY_H

#include "Sigil2/Primitive.h" // PtrVal type
#include "Sigil2/SigiLog.hpp"

#include <limits>
#include <vector>
#include <memory>

/* Shadow Memory tracks 'shadow state' for an address.
 * For further clarification, please read,
 * "How to Shadow Every Byte of Memory Used by a Program"
 * by Nicholas Nethercote and Julian Seward
 */

using Addr = PtrVal;

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
    using SecondaryMapPtr = std::unique_ptr<SecondaryMap>;
    using PrimaryMap = std::vector<SecondaryMapPtr>;

    auto operator[](Addr addr) -> SO&
    {
        if ((addr >> addr_bits) == 0)
        {
            SecondaryMapPtr &ptr = pm[addr >> sm_bits]; /* PM offset */
            if (ptr == nullptr)
            {
                ptr = SecondaryMapPtr(new SecondaryMap(sm_size));
                assert(ptr != nullptr);
            }

            return (*ptr)[addr & ((1ULL << sm_bits) - 1)]; /* SM offset */
        }
        else
        {
            char s_addr[32];
            sprintf(s_addr, "0x%lx", addr);
            std::string msg("shadow memory max address limit [");
            msg.append(s_addr).append("]");
            SigiLog::fatal(msg);
        }
    }

  private:
    PrimaryMap pm;

};

#endif

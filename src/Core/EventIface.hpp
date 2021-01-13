#ifndef PRISM_EVENTIFACE_H
#define PRISM_EVENTIFACE_H

#include "Primitive.h"
#include "EventCapability.hpp"

namespace prism
{
//-----------------------------------------------------------------------------
// Lightweight accessors

struct MemEvent
/* Memory Event API */
{
    MemEvent(const unsigned char* buf) : buf(buf){}

    auto type() const {
        const auto mem_ev_slot1 = reinterpret_cast<const PrismMemEvSlot_1*>(buf);
        return mem_ev_slot1->mem_ty;
    }

    auto isLoad() const -> bool {
        constexpr unsigned char ty_mask = (0b000'11'000);
        constexpr unsigned char load_shifted = (MemTypeEnum::PRISM_MEM_LOAD << 3);
        return (*buf & ty_mask) == load_shifted;
    }

    auto isStore() const -> bool {
        constexpr unsigned char ty_mask = (0b000'11'000);
        constexpr unsigned char store_shifted = (MemTypeEnum::PRISM_MEM_STORE << 3);
        return (*buf & ty_mask) == store_shifted;
    }

    auto addr() const {
        const auto mem_ev_slot2 = reinterpret_cast<const PrismMemEvSlot_2*>(buf);
        return mem_ev_slot2->mem_addr;
    }

    auto id() const {
        return *(buf+8);
    }

    auto accessed_bytes() const -> uint32_t {
        constexpr unsigned char size_mask = (0b000'00'111);
        const unsigned char sz = *buf & size_mask;
        return 1U << sz;
    }

    auto accessed_bits_enumval() const {
        constexpr unsigned char size_mask = (0b000'00'111);
        return *buf & size_mask;
    }

    const unsigned char* buf{nullptr};
};


//-----------------------------------------------------------------------------
/* Compute Event API */
struct CompEvent
{
    CompEvent(const unsigned char* buf) : buf(buf) {}

    auto type() const {
        return *(buf+1) >> 2;
    }

    auto width_bytes() const {
        constexpr unsigned char size_mask = (0b000'00'111);
        const unsigned char sz = *buf & size_mask;
        return 1U << sz;
    }

    auto width_bits_enumval() const {
        constexpr unsigned char size_mask = (0b000'00'111);
        return *buf & size_mask;
    }

    auto isIOP() const -> bool {
        constexpr unsigned char fmt_mask = (0b000'11'000);
        constexpr unsigned char iop_shifted = (CompCostTypeEnum::PRISM_COMP_IOP << 3);
        return (*buf & fmt_mask) == iop_shifted;
    }

    auto isFLOP() const -> bool {
        constexpr unsigned char fmt_mask = (0b000'11'000);
        constexpr unsigned char flop_shifted = (CompCostTypeEnum::PRISM_COMP_FLOP << 3);
        return (*buf & fmt_mask) == flop_shifted;
    }

    auto arity() const {
        constexpr unsigned char arity_mask = (0b0000'0011);
        return (*(buf+1) & arity_mask) + 1;
    }

    auto id(uint8_t loc) const {
        return (*(buf+2+loc));
    }

    const unsigned char* buf{nullptr};
};


//-----------------------------------------------------------------------------
/* Context Event API */
struct CxtEvent
{
    CxtEvent(const unsigned char *buf) : buf(buf) {}
    static constexpr unsigned char ty_mask = 0b000'11111;

    auto type() const {
        return *buf & ty_mask;
    }

    static constexpr auto type(const unsigned char* buf) {
        return *buf & ty_mask;
    }

    auto insn_addr() const {
        const uintptr_t addr = *reinterpret_cast<const uintptr_t*>(buf);
        return addr;
    }

    auto cxt_name() const {
        return reinterpret_cast<const char*>(buf+2);
    }

    auto cxt_name_len() const {
        return *(buf+1) + 1;
    }

    const unsigned char* buf{nullptr};
};


//-----------------------------------------------------------------------------
/* Sync Event API */
struct SyncEvent
{
    SyncEvent(const unsigned char* buf) : buf(buf) {}
    static constexpr unsigned char ty_mask = 0b000'11111;

    auto type() const {
        return *buf & ty_mask;
    }

    static constexpr auto type(const unsigned char* buf) {
        return *buf & ty_mask;
    }

    auto data() const -> uintptr_t {
        return *reinterpret_cast<const uintptr_t*>(buf+1);
    }

    auto dataExtra() const -> uintptr_t {
        return *reinterpret_cast<const uintptr_t*>(buf+1+sizeof(uintptr_t));
    }

    const unsigned char* buf{nullptr};
};


//-----------------------------------------------------------------------------
/* Event Configuration */
struct EventStreamParserConfig {
    std::byte cfg_bitvector_mem_ev{0};
    std::byte cfg_bitvector_comp_ev{0};
    std::byte cfg_bitvector_sync_ev{0};
    std::byte cfg_bitvector_cxt_ev{0};
    bool comp_arity_enabled{false};
    bool comp_ids_enabled{false};

    static constexpr uint32_t cfg_bytes_cfg_ev{2};
    uint32_t cfg_bytes_mem_ev{0};
    uint32_t cfg_bytes_comp_ev{0};
    static constexpr uint32_t cfg_bytes_sync_ev{1};
    static constexpr uint32_t cfg_bytes_cxt_ev{1};
    static constexpr uint32_t cfg_bytes_cf_ev{0};

    EventStreamParserConfig() {}
    EventStreamParserConfig(const capability::EvGenCaps& caps);

    uint32_t comp_id_bytes(const std::byte* comp_ev);
    void update_mem_evt_cfg(const std::byte cfg_byte);
    void update_comp_evt_cfg(const std::byte cfg_byte);
    void update_evt_cfgs(const unsigned char* buf);
};

} //end namespace prism

#endif

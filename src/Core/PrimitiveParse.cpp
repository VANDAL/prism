#include <cstddef>
#include <cstdint>
#include <array>
#include "PrimitiveEnums.h"

enum EventTypeEnum {
    PRISM_EVENTTYPE_MEM = 0,
    PRISM_EVENTTYPE_COMP,
    PRISM_EVENTTYPE_SYNC,
    PRISM_EVENTTYPE_CXT,
    PRISM_EVENTTYPE_CF,
    PRISM_EVENTTYPE_CFG,
    PRISM_EVENTTYPE_COUNT,
};
#define PRISM_EVENT_TYPE_BITS ( 3 )

/** Config event bit sizes, in order */
#define PRISM_CFG_EVTYPE_BITS ( 3 )
#define PRISM_CFG_TOTAL_BITS ( 8 )
struct PrismCfgEvSlot_1 {
    unsigned char ev_ty : PRISM_EVENT_TYPE_BITS;
    unsigned char cfg_ty : PRISM_CFG_EVTYPE_BITS;

    // reserved, can be used for certain directives w/ metadata.
    // One example is growing and remapping a 'name' heap if we want to implement a shared memory heap
    // for sending function names via offsets into the heap.
    // This is in in contrast to what we do right now which is copying the entire C string each time.
    unsigned char : (8 - PRISM_EVENT_TYPE_BITS + PRISM_CFG_EVTYPE_BITS);
};
struct PrismCfgEvSlot_2 {
    unsigned char cfg : PRISM_CFG_TOTAL_BITS;
};
#define PRISM_CFG_SYNC_BITS ( 0 )
#define PRISM_CFG_CXT_BITS ( 0 )

/** Memory event bit sizes, in order */
#define PRISM_MEM_TYPE_BITS ( 2 )
#define PRISM_MEM_SIZE_BITS ( 3 )
#define PRISM_MEM_ADDR_BITS ( 56 )
#define PRISM_MEM_ID_BITS ( 8 )
struct PrismMemEvSlot_1 {
    unsigned char ev_ty : PRISM_EVENT_TYPE_BITS;
    unsigned char mem_ty : PRISM_MEM_TYPE_BITS;
    unsigned char mem_sz : PRISM_MEM_SIZE_BITS;
};
struct PrismMemEvSlot_2 {
    /** TODO  check endianness order */
    uintptr_t : 8;
    uintptr_t mem_addr : PRISM_MEM_ADDR_BITS;
};
constexpr uint32_t prismMemEvBytes_forCfg(std::byte cfg) {
    const std::byte mask = std::byte{0b0011'0000} & cfg;
    if (mask == std::byte{0b0011'0000}) {
        return 9;
    } else if (mask == std::byte{0b0010'0000}) {
        return 8;
    } else if (mask == std::byte{0b0001'0000}) {
        return 2;
    } else {
        return 1;
    }
}

/** Compute event bit sizes, in order */
#define PRISM_COMP_FMT_BITS ( 2 )
#define PRISM_COMP_SIZE_BITS ( 3 )
#define PRISM_COMP_TYPE_BITS ( 8 )
#define PRISM_COMP_ARITY_BITS ( 2 )
#define PRISM_COMP_RESERVED_SLOT1_BITS ( 6 )
struct PrismCompEvSlot_1 {
    unsigned char ev_ty : PRISM_EVENT_TYPE_BITS;
    unsigned char op_fmt : PRISM_COMP_FMT_BITS;
    unsigned char op_sz : PRISM_COMP_SIZE_BITS;
};
struct PrismCompEvSlot_2 {
    unsigned char op_ty : PRISM_COMP_TYPE_BITS;
};
struct PrismCompEvSlot_3 {
    unsigned char op_arity : PRISM_COMP_ARITY_BITS;
    unsigned char reserved : PRISM_COMP_RESERVED_SLOT1_BITS;
};
constexpr uint32_t prismCompEvBytes_forCfg(std::byte cfg) {
    // Note compute events have variable length depending on arity
    // e.g. if IDs are enabled, and arity is 3, then there will by 3 extra bytes
    // This extra space isn't accounted for in this function, since
    // it depends on the dynamic runtime value of the arity field.
    const std::byte mask = std::byte{0b0011'0000} & cfg;
    if (mask == std::byte{0b0011'0000}) {
        return 3;
    } else if (mask == std::byte{0b0010'0000}) {
        return 2;
    } else if (mask == std::byte{0b0001'0000}) {
        return 2;
    } else {
        return 1;
    }
}

#define PRISM_CXT_TYPE_BITS ( 5 )
struct PrismCxtEvSlot_1 {
    unsigned char ev_ty : PRISM_EVENT_TYPE_BITS;
    unsigned char cxt_ty : PRISM_CXT_TYPE_BITS;
};

/** Synchronization event bit sizes, in order */
#define PRISM_SYNC_TYPE_BITS ( 5 )
struct PrismSyncEvSlot_1 {
    unsigned char ev_ty : PRISM_EVENT_TYPE_BITS;
    unsigned char sync_ty : PRISM_SYNC_TYPE_BITS;
};


/** Current configutations */
namespace prism {
std::byte cfg_byte_mem_ev{};
std::byte cfg_byte_comp_ev{};
std::byte cfg_byte_sync_ev{};
std::byte cfg_byte_cxt_ev{};

constexpr uint32_t cfg_bytes_cfg_ev{2};
uint32_t cfg_bytes_mem_ev{};
uint32_t cfg_bytes_comp_ev{};
uint32_t cfg_bytes_sync_ev{};
uint32_t cfg_bytes_cxt_ev{};
constexpr uint32_t cfg_bytes_cf_ev{0};
} // end namespace prism


/** Look up tables */


/** byte offset/ bit shift mapping */
/** So depends on enabled configs */


/* Event Configuration */
struct EventConfigState {
    std::byte cfg_bitvector_mem_ev{};
    std::byte cfg_bitvector_comp_ev{};
    std::byte cfg_bitvector_sync_ev{};
    std::byte cfg_bitvector_cxt_ev{};

    static constexpr uint32_t cfg_bytes_cfg_ev{2};
    uint32_t cfg_bytes_mem_ev{};
    uint32_t cfg_bytes_comp_ev{};
    static constexpr uint32_t cfg_bytes_sync_ev{1};
    static constexpr uint32_t cfg_bytes_cxt_ev{1};
    static constexpr uint32_t cfg_bytes_cf_ev{0};

    void update_evt_cfgs(const char* by) {
        const PrismCfgEvSlot_1* evt_cfg_slot1 = reinterpret_cast<const PrismCfgEvSlot_1*>(by);
        const PrismCfgEvSlot_2* evt_cfg_slot2 = reinterpret_cast<const PrismCfgEvSlot_2*>(by+1);
        switch (evt_cfg_slot1->cfg_ty) {
        case PRISM_EVENTTYPE_MEM:
            cfg_bitvector_mem_ev = std::byte{evt_cfg_slot2->cfg};
            cfg_bytes_mem_ev = prismMemEvBytes_forCfg(cfg_bitvector_mem_ev);
            break;
        case PRISM_EVENTTYPE_COMP:
            cfg_bitvector_comp_ev = std::byte{evt_cfg_slot2->cfg};
            break;
        case PRISM_EVENTTYPE_SYNC:
            cfg_bitvector_sync_ev = std::byte{evt_cfg_slot2->cfg};
            break;
        case PRISM_EVENTTYPE_CXT:
            cfg_bitvector_cxt_ev = std::byte{evt_cfg_slot2->cfg};
            break;
        case PRISM_EVENTTYPE_CF:
            break;
        default:
            break;
        }
    }
};


struct MemEvent
{
    // TODO assert configs are valid on each access, or let the user check?
    // Assert at first

    MemEvent(const unsigned char* buf) : buf(buf){
        // TODO assert not null
    }

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

    auto bytes() const -> uint32_t {
        constexpr unsigned char size_mask = (0b000'00'111);
        const unsigned char sz = *buf & size_mask;
        // TODO assert a valid value (not 0)
        return (1U << (sz - 1) );
    }

    const unsigned char* buf{nullptr};
};

struct CompEvent
{
    CompEvent(const unsigned char* buf) : buf(buf) {}

    auto type() const {
        const auto comp_ev_slot2 = reinterpret_cast<const PrismCompEvSlot_2*>(buf+1);
        return comp_ev_slot2->op_ty;
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

    const unsigned char* buf{nullptr};
};



struct CxtEvent
{
    CxtEvent(const unsigned char *buf) : buf(buf) {}

    auto type() const {
        const auto cxt_ev_slot1 = reinterpret_cast<const PrismCxtEvSlot_1*>(buf);
        return cxt_ev_slot1->cxt_ty;
    }

    auto insn_addr() const {
        const uintptr_t addr = *reinterpret_cast<const uintptr_t*>(buf+1);
        return addr;
    }

    auto fn_name() const {
        return reinterpret_cast<const char*>(buf+1);
    }

    const unsigned char* buf{nullptr};
};

struct SyncEvent
{
    SyncEvent(const unsigned char* buf) : buf(buf) {}

    auto type() const {
        const auto sync_ev_slot1 = reinterpret_cast<const PrismSyncEvSlot_1*>(buf);
        return sync_ev_slot1->sync_ty;
    }

    auto data() const -> uintptr_t {
        return *reinterpret_cast<const uintptr_t*>(buf+1);
    }

    auto dataExtra() const -> uintptr_t {
        return *reinterpret_cast<const uintptr_t*>(buf+1+sizeof(uintptr_t));
    }

    const unsigned char* buf{nullptr};
};


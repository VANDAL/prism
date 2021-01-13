#include "EventIface.hpp"

namespace prism
{
namespace
{
inline constexpr uint32_t prismCompEvBytes(std::byte cfg) {
    // Note compute events have variable length depending on arity
    // e.g. if IDs are enabled, and arity is 3, then there will by 3 extra bytes
    // This extra space isn't accounted for in this function, since
    // it depends on the dynamic runtime value of the arity field.
    const std::byte mask = std::byte{0b0011'0000} & cfg;
    if (mask != std::byte{0b0000'0000}) {
        return 2;
    } else {
        return 1;
    }
}

static constexpr uint32_t prismMemEvBytes(std::byte cfg) {
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
} // end namespace

EventStreamParserConfig::EventStreamParserConfig(const capability::EvGenCaps& caps)
{
    using namespace capability;
    std::byte cfg_byte{0};

    if (caps[PRISMCAP_MEMORY_LDST_TYPE] == availability::PRISMCAP_ENABLED)
        cfg_byte |= std::byte{0b1000'0000};
    if (caps[PRISMCAP_MEMORY_ACCESS_BYTES] == availability::PRISMCAP_ENABLED)
        cfg_byte |= std::byte{0b0100'0000};
    if (caps[PRISMCAP_MEMORY_ADDRESS] == availability::PRISMCAP_ENABLED)
        cfg_byte |= std::byte{0b0010'0000};
    if (caps[PRISMCAP_MEMORY_IDS] == availability::PRISMCAP_ENABLED)
        cfg_byte |= std::byte{0b0001'0000};
    update_mem_evt_cfg(cfg_byte);

    cfg_byte = std::byte{0};
    if (caps[PRISMCAP_COMPUTE_INT_OR_FLT] == availability::PRISMCAP_ENABLED)
        cfg_byte |= std::byte{0b1000'0000};
    if (caps[PRISMCAP_COMPUTE_WIDTH_BYTES] == availability::PRISMCAP_ENABLED)
        cfg_byte |= std::byte{0b0100'0000};
    if (caps[PRISMCAP_COMPUTE_OP_TYPE] == availability::PRISMCAP_ENABLED)
        cfg_byte |= std::byte{0b0010'0000};
    if (caps[PRISMCAP_COMPUTE_ARITY] == availability::PRISMCAP_ENABLED)
        cfg_byte |= std::byte{0b0001'0000};
    if (caps[PRISMCAP_COMPUTE_IDS] == availability::PRISMCAP_ENABLED)
        cfg_byte |= std::byte{0b0000'1000};
    update_comp_evt_cfg(cfg_byte);
}

uint32_t EventStreamParserConfig::comp_id_bytes(const std::byte* comp_ev) {
    if (comp_arity_enabled && comp_ids_enabled) {
        uint8_t arity = std::to_integer<uint8_t>(*(comp_ev+1) & std::byte{3}) + 1;
        return arity + cfg_bytes_comp_ev;
    } else {
        return cfg_bytes_comp_ev;
    }
}

void EventStreamParserConfig::update_mem_evt_cfg(const std::byte cfg_byte) {
    cfg_bitvector_mem_ev = cfg_byte;
    cfg_bytes_mem_ev = prismMemEvBytes(cfg_byte);
}

void EventStreamParserConfig::update_comp_evt_cfg(const std::byte cfg_byte) {
    cfg_bitvector_comp_ev = cfg_byte;
    cfg_bytes_comp_ev = prismCompEvBytes(cfg_byte);
    if ((cfg_bitvector_comp_ev & std::byte{0b0001'0000}) != std::byte{0}) {
        comp_arity_enabled = true;
        if ((cfg_bitvector_comp_ev & std::byte{0b0000'1000}) != std::byte{0})
            comp_ids_enabled = true;
    }
}

void EventStreamParserConfig::update_evt_cfgs(const unsigned char* buf) {
    const PrismCfgEvSlot_1* evt_cfg_slot1 = reinterpret_cast<const PrismCfgEvSlot_1*>(buf);
    std::byte cfg_byte = std::byte{*(buf+1)};
    switch (evt_cfg_slot1->cfg_ty) {
    case PRISM_EVENTTYPE_MEM:
        update_mem_evt_cfg(cfg_byte);
        break;
    case PRISM_EVENTTYPE_COMP:
        update_comp_evt_cfg(cfg_byte);
        break;
    case PRISM_EVENTTYPE_SYNC:
        cfg_bitvector_sync_ev = cfg_byte;
        break;
    case PRISM_EVENTTYPE_CXT:
        cfg_bitvector_cxt_ev = cfg_byte;
        break;
    case PRISM_EVENTTYPE_CF:
        break;
    default:
        break;
    }
}

} // end namespace prism

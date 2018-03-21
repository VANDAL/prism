#include "Handler.hpp"
#include "spdlog/spdlog.h"
#include <iostream>
#include <atomic>

namespace
{
std::atomic<unsigned long> global_read_cnt{0};
std::atomic<unsigned long> global_write_cnt{0};
std::atomic<unsigned long> global_mem_cnt{0};
std::atomic<unsigned long> global_iop_cnt{0};
std::atomic<unsigned long> global_flop_cnt{0};
std::atomic<unsigned long> global_comp_cnt{0};
std::atomic<unsigned long> global_swap_cnt{0};
std::atomic<unsigned long> global_sync_cnt{0};
std::atomic<unsigned long> global_cf_cnt{0};
std::atomic<unsigned long> global_instr_cnt{0};
std::atomic<unsigned long> global_cxt_cnt{0};
std::atomic<unsigned long> global_spawn_cnt{0};
std::atomic<unsigned long> global_join_cnt{0};
std::atomic<unsigned long> global_lock_cnt{0};
std::atomic<unsigned long> global_unlock_cnt{0};
std::atomic<unsigned long> global_barrier_cnt{0};
std::atomic<unsigned long> global_wait_cnt{0};
std::atomic<unsigned long> global_sig_cnt{0};
std::atomic<unsigned long> global_broad_cnt{0};
};

namespace SimpleCount
{

auto Handler::onSyncEv(const prism::SyncEvent &ev) -> void
{
    if (ev.type() == PRISM_SYNC_SWAP)
    {
        ++swap_cnt;
    }
    else
    {
        ++sync_cnt;
        switch(ev.type())
        {
        case PRISM_SYNC_CREATE:
            ++spawn_cnt;
            break;
        case PRISM_SYNC_JOIN:
            ++join_cnt;
            break;
        case PRISM_SYNC_LOCK:
            ++lock_cnt;
            break;
        case PRISM_SYNC_UNLOCK:
            ++unlock_cnt;
            break;
        case PRISM_SYNC_BARRIER:
            ++barrier_cnt;
            break;
        case PRISM_SYNC_CONDWAIT:
            ++wait_cnt;
            break;
        case PRISM_SYNC_CONDSIG:
            ++sig_cnt;
            break;
        case PRISM_SYNC_CONDBROAD:
            ++broad_cnt;
            break;

        }
    }
}


auto Handler::onCompEv(const prism::CompEvent &ev) -> void
{
    ++comp_cnt;
    if (ev.isIOP())
        ++iop_cnt;
    else if (ev.isFLOP())
        ++flop_cnt;
}


auto Handler::onMemEv(const prism::MemEvent &ev) -> void
{
    ++mem_cnt;
    if (ev.isStore())
        ++write_cnt;
    else if (ev.isLoad())
        ++read_cnt;
}


auto Handler::onCFEv(const PrismCFEv &ev) -> void
{
    ++cf_cnt;
}


auto Handler::onCxtEv(const prism::CxtEvent &ev) -> void
{
    ++cxt_cnt;
    if (ev.type() == PRISM_CXT_INSTR)
        ++instr_cnt;
}


Handler::~Handler()
{
    global_read_cnt    += read_cnt;
    global_write_cnt   += write_cnt;
    global_mem_cnt     += mem_cnt;
    global_iop_cnt     += iop_cnt;
    global_flop_cnt    += flop_cnt;
    global_comp_cnt    += comp_cnt;
    global_swap_cnt    += swap_cnt;
    global_sync_cnt    += sync_cnt;
    global_cf_cnt      += cf_cnt;
    global_cxt_cnt     += cxt_cnt;
    global_instr_cnt   += instr_cnt;
    global_spawn_cnt   += spawn_cnt;
    global_join_cnt    += join_cnt;
    global_lock_cnt    += lock_cnt;
    global_unlock_cnt  += unlock_cnt;
    global_barrier_cnt += barrier_cnt;
    global_wait_cnt    += wait_cnt;
    global_sig_cnt     += sig_cnt;
    global_broad_cnt   += broad_cnt;
}


auto cleanup() -> void
{
    std::shared_ptr<spdlog::logger> logger = spdlog::stdout_logger_st("simplecount-console");
    logger->set_pattern("[SimpleCount] %v");

    logger->info("Total Compute   Events: {}", std::to_string(global_comp_cnt));
    logger->info("Total IOP       Events: {}", std::to_string(global_iop_cnt));
    logger->info("Total FLOP      Events: {}", std::to_string(global_flop_cnt));
    logger->info("Total Memory    Events: {}", std::to_string(global_mem_cnt));
    logger->info("Total ReadMem   Events: {}", std::to_string(global_read_cnt));
    logger->info("Total WriteMem  Events: {}", std::to_string(global_write_cnt));
    logger->info("Total Swap      Events: {}", std::to_string(global_swap_cnt));
    logger->info("Total Sync      Events: {}", std::to_string(global_sync_cnt));
    logger->info("Total Spawn     Events: {}", std::to_string(global_spawn_cnt));
    logger->info("Total Join      Events: {}", std::to_string(global_join_cnt));
    logger->info("Total Lock      Events: {}", std::to_string(global_lock_cnt));
    logger->info("Total Unlock    Events: {}", std::to_string(global_unlock_cnt));
    logger->info("Total Barrier   Events: {}", std::to_string(global_barrier_cnt));
    logger->info("Total Wait      Events: {}", std::to_string(global_wait_cnt));
    logger->info("Total Signal    Events: {}", std::to_string(global_sig_cnt));
    logger->info("Total Broadcast Events: {}", std::to_string(global_broad_cnt));
    logger->info("Total CntlFlow  Events: {}", std::to_string(global_cf_cnt));
    logger->info("Total Instr     Events: {}", std::to_string(global_instr_cnt));
    logger->info("Total Context   Events: {}", std::to_string(global_cxt_cnt));
}


auto requirements() -> prism::capabilities
{
    using namespace prism;
    using namespace prism::capability;

    auto caps = initCaps();

    caps[MEMORY]         = availability::enabled;
    caps[MEMORY_LDST]    = availability::enabled;
    caps[MEMORY_SIZE]    = availability::disabled;
    caps[MEMORY_ADDRESS] = availability::disabled;

    caps[COMPUTE]              = availability::enabled;
    caps[COMPUTE_INT_OR_FLOAT] = availability::enabled;
    caps[COMPUTE_ARITY]        = availability::disabled;
    caps[COMPUTE_OP]           = availability::disabled;
    caps[COMPUTE_SIZE]         = availability::disabled;

    caps[CONTROL_FLOW] = availability::disabled;

    caps[SYNC]      = availability::enabled;
    caps[SYNC_TYPE] = availability::enabled;
    caps[SYNC_ARGS] = availability::disabled;

    caps[CONTEXT_INSTRUCTION] = availability::enabled;
    caps[CONTEXT_BASIC_BLOCK] = availability::disabled;
    caps[CONTEXT_FUNCTION]    = availability::disabled;
    caps[CONTEXT_THREAD]      = availability::enabled;

    return caps;
}
}; //end namespace SimpleCount

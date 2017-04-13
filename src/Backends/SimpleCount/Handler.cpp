#include "Handler.hpp"
#include "spdlog/spdlog.h"
#include <iostream>
#include <atomic>

namespace
{
std::atomic<unsigned long> global_mem_cnt;
std::atomic<unsigned long> global_comp_cnt;
std::atomic<unsigned long> global_sync_cnt;
std::atomic<unsigned long> global_cf_cnt;
std::atomic<unsigned long> global_cxt_cnt;
};

namespace SimpleCount
{

auto Handler::onSyncEv(const SglSyncEvWrapper &ev) -> void
{
    ++sync_cnt;
}


auto Handler::onCompEv(const SglCompEvWrapper &ev) -> void
{
    ++comp_cnt;
}


auto Handler::onMemEv(const SglMemEvWrapper &ev) -> void
{
    ++mem_cnt;
}


auto Handler::onCFEv(const SglCFEv &ev) -> void
{
    ++cf_cnt;
}


auto Handler::onCxtEv(const SglCxtEvWrapper &ev) -> void
{
    ++cxt_cnt;
}


Handler::~Handler()
{
    global_mem_cnt += mem_cnt;
    global_comp_cnt += comp_cnt;
    global_sync_cnt += sync_cnt;
    global_cf_cnt += cf_cnt;
    global_cxt_cnt += cxt_cnt;
}


auto cleanup() -> void
{
    std::shared_ptr<spdlog::logger> logger = spdlog::stdout_logger_st("simplecount-console");
    logger->set_pattern("[SimpleCount] %v");

    logger->info("Total Compute  Events: {}", global_comp_cnt);
    logger->info("Total Memory   Events: {}", global_mem_cnt);
    logger->info("Total Sync     Events: {}", global_sync_cnt);
    logger->info("Total CntlFlow Events: {}", global_cf_cnt);
    logger->info("Total Context  Events: {}", global_cxt_cnt);
}
}; //end namespace SimpleCount

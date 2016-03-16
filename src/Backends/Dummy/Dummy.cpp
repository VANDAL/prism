#include "Dummy.hpp"
#include "spdlog.h"

namespace
{
unsigned long mem_cnt;
unsigned long comp_cnt;
unsigned long sync_cnt;
};

namespace dummy
{
void countMems(SglMemEv ev)
{
	++mem_cnt;
}

void countComps(SglCompEv ev)
{
	++comp_cnt;
}

void countSyncs(SglSyncEv ev)
{
	++sync_cnt;
}

void cleanup()
{
	std::shared_ptr<spdlog::logger> logger = spdlog::stdout_logger_st("dummy-console");
	logger->set_pattern("[Dummy] %v");

	logger->info("Total Memory Events: {}", mem_cnt);
	logger->info("Total Compute Events: {}", comp_cnt);
	logger->info("Total Synchronization Events: {}", sync_cnt); 
}
}; //end namespace dummy

#include "LogHelpers.hpp"
#include "spdlog.h"

#include <sstream>

namespace STGen
{
/**
 * The event log is updated often.
 * Directly using spdlog for performance, instead of Sigil's
 * logging abstraction. //TODO get rid of SglLog?
 *
 * The log is created and set up by changing the current thread
 * to a thread not seen before.
 *
 * Each ST event type flushes itself to the specific log file.
 */
static std::vector<std::shared_ptr<spdlog::logger>> loggers(8);
std::shared_ptr<spdlog::logger> curr_logger;

void initThreadLog(TId tid)
{
	assert( tid >= 0 );

	char filename[32] = "sigil.events-";
	sprintf(filename, "%s%u.txt", filename, tid);

	spdlog::set_async_mode(4096);
	spdlog::create<spdlog::sinks::simple_file_sink_st>(filename, filename);
	spdlog::get(filename)->set_pattern("%v");


	if ( static_cast<int>(loggers.size()) <= tid )
	{
		loggers.resize(loggers.size()*2, nullptr);
	}
	loggers[tid] = spdlog::get(filename);

	curr_logger = loggers[tid];
}

void switchThreadLog(TId tid)
{
	assert( static_cast<int>(loggers.size()) > tid );
	assert( loggers[tid] != nullptr );

	curr_logger = loggers[tid];
}

void logCompEvent(const STCompEvent& ev)
{
	std::stringstream logmsg;
	logmsg << ev.curr_event_id
		<< "," << ev.curr_thread_id 
		<< "," << ev.iop_cnt
		<< "," << ev.flop_cnt
		<< "," << ev.load_cnt
		<< "," << ev.store_cnt;

	/* log write addresses */
	for (auto& addr_pair : ev.stores_unique.ranges)
	{
		logmsg << "$" //unique write delimiter
			<< addr_pair.first 
			<< " "
			<< addr_pair.second;
	}

	/* log read addresses */
	for (auto& addr_pair : ev.loads_unique.ranges)
	{
		logmsg << "*" //unique read delimiter
			<< addr_pair.first 
			<< " "
			<< addr_pair.second;
	}

	curr_logger->info(logmsg.str());
}

void logCommEdgeEvent(const STCommEvent& ev)
{
	std::stringstream logmsg;
	logmsg << ev.curr_event_id
		<< "," << ev.curr_thread_id;

	for (auto& edge_tuple : ev.comms)
	{
		logmsg << "#" << std::get<0>(edge_tuple)
			<< "," << std::get<1>(edge_tuple)
			<< "," << std::get<2>(edge_tuple)
			<< "," << std::get<3>(edge_tuple);
	}

	curr_logger->info(logmsg.str());
}

void logSyncEvent(TId tid, EId eid, UChar type, Addr addr)
{
	std::stringstream logmsg;
	logmsg << eid
		<< "," << tid
		<< "," << "pth_ty:"
		<< (int)type
		<< "^"
		<< addr;
	curr_logger->info(logmsg.str());
}
}; //end namespace STGen

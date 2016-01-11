#include "LogHelpers.hpp"
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

		char filename[16] = "sigil.events-";
		sprintf(filename, "%s%u", filename, tid);

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

	void logCompEvent(STCompEvent& ev)
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
			logmsg << " $ " //unique write delimiter
				<< addr_pair.first 
				<< " "
				<< addr_pair.second;
		}

		/* log read addresses */
		for (auto& addr_pair : ev.loads_unique.ranges)
		{
			logmsg << " * " //unique read delimiter
				<< addr_pair.first 
				<< " "
				<< addr_pair.second;
		}

		curr_logger->info(logmsg.str());
	}

	void logCommEdgeEvent()
	{
	}

	void logSyncEvent(TId tid, EId eid, UChar type, Addr addr)
	{
		std::stringstream logmsg;

		curr_logger->info(logmsg.str());
		logmsg << eid
			<< "," << tid
			<< "," << "pth_ty:"
			<< "," << type
			<< "," << "^"
			<< "," << addr;
		curr_logger->info(logmsg.str());
	}
};

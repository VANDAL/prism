#include "STEvent.hpp"

#include "spdlog.h"
#include <sstream>
#include <cassert>

namespace STGen
{

////////////////////////////////////////////////////////////
// SynchroTrace - Logging
////////////////////////////////////////////////////////////
static std::vector<std::shared_ptr<spdlog::logger>> loggers(16);
std::shared_ptr<spdlog::logger> curr_logger = nullptr;

namespace
{
void initThreadLog(TId tid)
{
	assert( tid >= 0 );

	std::string thread_filename = filename + std::to_string(tid);

	spdlog::set_async_mode(8192);
	spdlog::create<spdlog::sinks::simple_file_sink_st>(thread_filename, thread_filename);
	spdlog::get(thread_filename)->set_pattern("%v");

	if ( static_cast<int>(loggers.size()) <= tid )
	{
		loggers.resize(loggers.size()*2, nullptr);
	}
	loggers[tid] = spdlog::get(thread_filename);

	curr_logger = loggers[tid];
}

void switchThreadLog(TId tid)
{
	assert( static_cast<int>(loggers.size()) > tid );
	assert( loggers[tid] != nullptr );

	curr_logger = loggers[tid];
}
}; //end namespace

////////////////////////////////////////////////////////////
// SynchroTrace - Events
////////////////////////////////////////////////////////////
std::unordered_map<TId, EId> STEvent::event_ids;
TId STEvent::curr_thread_id = -1;
EId STEvent::curr_event_id = -1;

void STEvent::flush()
{
	if ( is_active )
	{
		detailedFlush();
		curr_event_id++;
	}
}

void STEvent::setThread(TId tid)
{
	assert( tid >= 0 );

	if ( curr_thread_id == tid )
	{
		return;
	}

	event_ids[curr_thread_id] = curr_event_id;
	if/*new thread*/( event_ids.find(tid) == event_ids.cend() )
	{
		event_ids[tid] = 0;
		curr_event_id = 0;

		/* start log file for this thread */
		initThreadLog(tid);
	}
	else
	{
		curr_event_id = event_ids[tid];
		switchThreadLog(tid);
	}

	curr_thread_id = tid;
}
	
////////////////////////////////////////////////////////////
// SynchroTrace - Compute Event
////////////////////////////////////////////////////////////
STCompEvent::STCompEvent()
{
	reset();
}

void STCompEvent::detailedFlush()
{
	std::stringstream logmsg;
	logmsg << curr_event_id
		<< "," << curr_thread_id 
		<< "," << iop_cnt
		<< "," << flop_cnt
		<< "," << load_cnt
		<< "," << store_cnt
		<< std::hex;

	/* log write addresses */
	for (auto& addr_pair : stores_unique.ranges)
	{
		logmsg << " $ " //unique write delimiter
			<< addr_pair.first 
			<< " "
			<< addr_pair.second;
	}

	/* log read addresses */
	for (auto& addr_pair : loads_unique.ranges)
	{
		logmsg << " * " //unique read delimiter
			<< addr_pair.first 
			<< " "
			<< addr_pair.second;
	}

	curr_logger->info(logmsg.str());
	reset();
}

bool STCompEvent::updateWrites(Addr begin, Addr size)
{
	is_active = true;
	total_events++;
	return stores_unique.insert(begin, begin+size-1);
}

bool STCompEvent::updateWrites(SglMemEv ev)
{
	is_active = true;
	total_events++;
	return stores_unique.insert(ev.begin_addr, ev.begin_addr+ev.size-1);
}

bool STCompEvent::updateReads(Addr begin, Addr size)
{
	is_active = true;
	total_events++;
	return loads_unique.insert(begin, begin+size-1);
}

bool STCompEvent::updateReads(SglMemEv ev)
{
	is_active = true;
	total_events++;
	return loads_unique.insert(ev.begin_addr, ev.begin_addr+ev.size-1);
}

void STCompEvent::incIOP()
{
	is_active = true;
	iop_cnt++;
	total_events++;
}

void STCompEvent::incFLOP()
{
	is_active = true;
	flop_cnt++;
	total_events++;
}

void STCompEvent::reset()
{
	iop_cnt = 0;
	flop_cnt = 0;
	store_cnt = 0;
	load_cnt = 0;
	total_events = 0;
	stores_unique.clear();
	loads_unique.clear();

	is_active = false;
}

////////////////////////////////////////////////////////////
// SynchroTrace - Communication Event
////////////////////////////////////////////////////////////
STCommEvent::STCommEvent()
{
	reset();
}

void STCommEvent::detailedFlush()
{
	std::stringstream logmsg;
	logmsg << curr_event_id
		<< "," << curr_thread_id;

	/* log comm edges between current and other threads */
	for (auto& edge : comms)
	{
		logmsg << " # " //unique write delimiter
			<< std::get<0>(edge) 
			<< " " << std::get<1>(edge)
			<< std::hex
			<< " " << std::get<2>(edge)
			<< " " << std::get<3>(edge)
			<< std::dec;
	}

	curr_logger->info(logmsg.str());
	reset();
}

void STCommEvent::addEdge(TId writer, EId writer_event, Addr addr)
{
	is_active = true;
	if (comms.empty())
	{
		comms.push_back(std::make_tuple(writer, writer_event, addr, addr)); //new edge
	}
	else 
	{
		auto& last_writer = std::get<0>(comms.back());
		if/*read from same thread*/( writer == last_writer )
		{
			auto& last_addr = std::get<3>(comms.back());
			last_addr = addr;
		}
		else/*new thread*/
		{
			comms.push_back(std::make_tuple(writer, writer_event, addr, addr)); //new edge
		}
	}
}

void STCommEvent::reset()
{
	comms.clear();
	is_active = false;
}
	
////////////////////////////////////////////////////////////
// SynchroTrace - Synchronization Event
////////////////////////////////////////////////////////////
void STSyncEvent::logSync(UChar type, Addr sync_addr)
{
	std::stringstream logmsg;
	logmsg << curr_event_id
		<< "," << curr_thread_id
		<< "," << "pth_ty:"
		<< (int)type
		<< "^"
		<< std::hex << sync_addr;
	curr_logger->info(logmsg.str());
}

void STSyncEvent::detailedFlush()
{
	//nothing to flush
}

void STSyncEvent::reset()
{
	//nothing to reset
}
	
////////////////////////////////////////////////////////////
// Address Range
////////////////////////////////////////////////////////////

/* FIXME ML: occasionally I see addresses, in the traces, that are consecutive, 
 * but they're not getting condensed for some reason */
bool AddrRange::insert(Addr begin, Addr end)
{
	for(auto& range : ranges)
	{
		if ( begin == range.second+1 )
		{
			range.second = end;
			return true;
		}
		else if ( end+1 == range.first )
		{
			range.first = begin;
			return true;
		}
		else if (addrOverlap(begin, end, range.first, range.second))
		{
			return false; //memory addresses already stored
		}
		else if (addrOverlap(range.first, range.second, begin, end))
		{
			range.first = begin;
			range.second = end;
			return true; //replace the event with a superset of addresses
		}
		//TODO partial overlap not implemented, considered new address
	}

	ranges.push_back(std::make_pair(begin, end));
	return true;
}

bool AddrRange::addrOverlap(
				Addr addr1_begin, Addr addr1_end,
				Addr addr2_begin, Addr addr2_end
				)
{
	if (addr1_begin >= addr2_begin && addr1_end <= addr2_end)
		return true;
	else
		return false;
}

void AddrRange::clear()
{
	ranges.clear();
}
}; //end namespace STGen

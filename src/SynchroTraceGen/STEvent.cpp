#include "STEvent.hpp"

#include "spdlog.h"
#include <sstream>
#include <cassert>

namespace STGen
{

using std::stringstream;
using std::make_pair;
using std::make_tuple;
using std::get;
using std::hex;
using std::dec;

////////////////////////////////////////////////////////////
// SynchroTrace - Compute Event
////////////////////////////////////////////////////////////
STCompEvent::STCompEvent(TId &tid, EId &eid, const shared_ptr<spdlog::logger> &logger)
	: thread_id(tid)
	, event_id(eid)
	, logger(logger)
{
	reset();
}

void STCompEvent::flush()
{
	if (is_empty == false)
	{
		stringstream logmsg;
		logmsg << event_id
			<< "," << thread_id 
			<< "," << iop_cnt
			<< "," << flop_cnt
			<< "," << load_cnt
			<< "," << store_cnt
			<< hex;

		/* log write addresses */
		for (auto& addr_pair : stores_unique.get())
		{
			logmsg << " $ " //unique write delimiter
				<< addr_pair.first 
				<< " "
				<< addr_pair.second;
		}

		/* log read addresses */
		for (auto& addr_pair : loads_unique.get())
		{
			logmsg << " * " //unique read delimiter
				<< addr_pair.first 
				<< " "
				<< addr_pair.second;
		}

		logger->info(logmsg.str());
		event_id++;
		reset();
	}
}

void STCompEvent::updateWrites(Addr begin, Addr size)
{
	is_empty = false;
	total_events++;
	stores_unique.insert(make_pair(begin, begin+size-1));
}

void STCompEvent::updateWrites(SglMemEv ev)
{
	is_empty = false;
	total_events++;
	stores_unique.insert(make_pair(ev.begin_addr, ev.begin_addr+ev.size-1));
}

void STCompEvent::updateReads(Addr begin, Addr size)
{
	is_empty = false;
	total_events++;
	loads_unique.insert(make_pair(begin, begin+size-1));
}

void STCompEvent::updateReads(SglMemEv ev)
{
	is_empty = false;
	total_events++;
	loads_unique.insert(make_pair(ev.begin_addr, ev.begin_addr+ev.size-1));
}

void STCompEvent::incIOP()
{
	is_empty = false;
	iop_cnt++;
	total_events++;
}

void STCompEvent::incFLOP()
{
	is_empty = false;
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

	is_empty = true;
}

////////////////////////////////////////////////////////////
// SynchroTrace - Communication Event
////////////////////////////////////////////////////////////
STCommEvent::STCommEvent(TId &tid, EId &eid, const shared_ptr<spdlog::logger> &logger)
	: thread_id(tid)
	, event_id(eid)
	, logger(logger)
{
	reset();
}

void STCommEvent::flush()
{
	if (is_empty == false)
	{
		stringstream logmsg;
		logmsg << event_id
			<< "," << thread_id;

		/* log comm edges between current and other threads */
		for (auto& edge : comms)
		{
			logmsg << " # " //unique write delimiter
				<< get<0>(edge) 
				<< " " << get<1>(edge)
				<< hex
				<< " " << get<2>(edge)
				<< " " << get<3>(edge)
				<< dec;
		}

		logger->info(logmsg.str());
		reset();
	}
}

void STCommEvent::addEdge(TId writer, EId writer_event, Addr addr)
{
	is_empty = false;
	if (comms.empty())
	{
		comms.push_back(make_tuple(writer, writer_event, addr, addr)); //new edge
	}
	else 
	{
		auto& last_writer = get<0>(comms.back());
		if/*read from same thread*/( writer == last_writer )
		{
			auto& last_addr = get<3>(comms.back());
			last_addr = addr;
		}
		else/*new thread*/
		{
			comms.push_back(make_tuple(writer, writer_event, addr, addr)); //new edge
		}
	}
}

void STCommEvent::reset()
{
	comms.clear();
	is_empty = true;
}
	
////////////////////////////////////////////////////////////
// SynchroTrace - Synchronization Event
////////////////////////////////////////////////////////////
STSyncEvent::STSyncEvent(TId &tid, EId &eid, const shared_ptr<spdlog::logger> &logger)
	: thread_id(tid)
	, event_id(eid)
	, logger(logger) { }

void STSyncEvent::flush(UChar type, Addr sync_addr)
{
	stringstream logmsg;
	logmsg << event_id
		<< "," << thread_id
		<< "," << "pth_ty:"
		<< (int)type
		<< "^"
		<< hex << sync_addr;
	logger->info(logmsg.str());
}
	
////////////////////////////////////////////////////////////
// Unique Address Set
////////////////////////////////////////////////////////////
void STCompEvent::AddrSet::insert(const AddrRange &range)
{
/* XXX ML: profiled SynchroTraceGen and this function is
 * costing the majority of performance in the backend;
 * consider implementing a custom data structure */

/* TODO clean up flow control */

	assert (range.first <= range.second);

	/* insert if this is the first addr */
	if (ms.empty() == true)
	{
		ms.insert(range);
		return;
	}

	/* get the first addr pair that is not less than range */
	/* see http://en.cppreference.com/w/cpp/utility/pair/operator_cmp */
	auto it = ms.lower_bound(range);

	if (it != ms.cbegin())
	{
		if (it == ms.cend())
		/* if no address range starts at a higher address, 
		 * check the last element */
		{
			it = --ms.cend();
		}
		else
		/* check if the previous addr pair overlaps with range */
		{
			--it;
			if (range.first > it->second+1)
			{
				++it;
			}
		}
	}

	if (range.first == it->second+1)
	{
		/* extend 'it' by 'range' */
		auto tmp = make_pair(it->first, range.second);
		ms.erase(it);
		insert(tmp);
	}
	else if (range.second+1 == it->first)
	{
		/* extend 'it' by 'range' */
		auto tmp = make_pair(range.first, it->second);
		ms.erase(it);
		insert(tmp);
	}
	else if (range.first > it->second)
	{
		/* can't merge, just insert (at end) */
		ms.insert(range);
	}
	else if (range.first >= it->first)
	{
		if (range.second > it->second)
		/* extending 'it' to the end of 'range' */
		{
			/* merge, delete, and recheck; may overrun other addresses */
			auto tmp = make_pair(it->first, range.second);
			ms.erase(it);
			insert(tmp);
		}
		/* else do not insert; 'it' encompasses 'range' */
	}
	else /* if (range.first < it->first) */
	{
		if (range.second+1 == it->first)
		/* can append 'it' to 'range' */
		{
			/* merge, delete, and insert; no need to recheck */
			auto tmp = make_pair(range.first, it->second);
			ms.erase(it);
			ms.insert(tmp);
		}
		else if (range.second < it->first)
		/* no overlap */
		{
			/* nothing to merge */
			ms.insert(range);
		}
		else if(range.second <= it->second)
		/* begin address is extended */
		{
			/* merge, delete, and insert; no need to recheck */
			auto tmp = make_pair(range.first, it->second);
			ms.erase(it);
			ms.insert(tmp);
		}
		else /* if(range.second > it->second) */
		/* 'range' encompasses 'it' */
		{
			/* delete old range and insert bigger range */
			ms.erase(it);
			insert(range);
		}
	}
}

void STCompEvent::AddrSet::clear()
{
	ms.clear();
}
}; //end namespace STGen

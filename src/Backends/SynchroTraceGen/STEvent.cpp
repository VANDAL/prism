#include "STEvent.hpp"

#include "spdlog.h"
#include <cassert>

namespace STGen
{

////////////////////////////////////////////////////////////
// SynchroTrace - Compute Event
////////////////////////////////////////////////////////////
STCompEvent::STCompEvent(TId &tid, EId &eid, const std::shared_ptr<spdlog::logger> &logger,
		STInstrEvent &instr_ev)
	: instr_ev(instr_ev)
	, thread_id(tid)
	, event_id(eid)
	, logger(logger)
{
	reset();
}


void STCompEvent::flush()
{
	if(is_empty == false)
	{
		instr_ev.flush();

		std::stringstream logmsg;
		logmsg << event_id
			<< "," << thread_id
			<< "," << iop_cnt
			<< "," << flop_cnt
			<< "," << thread_local_load_cnt
			<< "," << thread_local_store_cnt
			<< std::hex << std::showbase;

		/* log write addresses */
		for(auto &addr_pair : stores_unique.get())
		{
			assert(addr_pair.first <= addr_pair.second);
			logmsg << " $ " //unique write delimiter
				<< addr_pair.first << " "
				<< addr_pair.second;
		}

		/* log read addresses */
		for(auto &addr_pair : loads_unique.get())
		{
			assert(addr_pair.first <= addr_pair.second);
			logmsg << " * " //unique read delimiter
				<< addr_pair.first << " "
				<< addr_pair.second;
		}

		logger->info(logmsg.str());
		++event_id;
		reset();
	}
}


void STCompEvent::updateWrites(const Addr begin, const Addr size)
{
	stores_unique.insert(std::make_pair(begin, begin+size-1));
}


void STCompEvent::updateWrites(const SglMemEv &ev)
{
	stores_unique.insert(std::make_pair(ev.begin_addr, ev.begin_addr+ev.size-1));
}


void STCompEvent::updateReads(const Addr begin, const Addr size)
{
	loads_unique.insert(std::make_pair(begin, begin+size-1));
}


void STCompEvent::updateReads(const SglMemEv &ev)
{
	loads_unique.insert(std::make_pair(ev.begin_addr, ev.begin_addr+ev.size-1));
}


void STCompEvent::incWrites()
{
	is_empty = false;
	++thread_local_store_cnt;
	++total_events;
}


void STCompEvent::incReads()
{
	is_empty = false;
	++thread_local_load_cnt;
	++total_events;
}


void STCompEvent::incIOP()
{
	is_empty = false;
	++iop_cnt;
	++total_events;
}


void STCompEvent::incFLOP()
{
	is_empty = false;
	++flop_cnt;
	++total_events;
}


inline void STCompEvent::reset()
{
	iop_cnt = 0;
	flop_cnt = 0;
	thread_local_store_cnt = 0;
	thread_local_load_cnt = 0;
	total_events = 0;
	stores_unique.clear();
	loads_unique.clear();

	is_empty = true;
}


////////////////////////////////////////////////////////////
// SynchroTrace - Communication Event
////////////////////////////////////////////////////////////
STCommEvent::STCommEvent(TId &tid, EId &eid, const std::shared_ptr<spdlog::logger> &logger,
		STInstrEvent &instr_ev)
	: instr_ev(instr_ev)
	, thread_id(tid)
	, event_id(eid)
	, logger(logger)
{
	reset();
}


void STCommEvent::flush()
{
	if(is_empty == false)
	{
		instr_ev.flush();

		std::stringstream logmsg;
		logmsg << event_id
			<< "," << thread_id;

		assert(comms.empty() == false);

		/* log comm edges between current and other threads */
		for(auto& edge : comms)
		{
			for(auto& addr_pair : std::get<2>(edge).get())
			{
				assert(addr_pair.first <= addr_pair.second);
				logmsg << " # "/*unique write delimiter*/
					<< std::get<0>(edge) << " "
					<< std::get<1>(edge) << " "
					<< std::hex << std::showbase
					<< addr_pair.first << " "
					<< addr_pair.second
					<< std::dec;
			}
		}

		logger->info(logmsg.str());
		++event_id;
		reset();
	}
}


void STCommEvent::addEdge(const TId writer, const EId writer_event, const Addr addr)
{
	is_empty = false;

	if(comms.empty())
	{
		comms.push_back(std::make_tuple(writer, writer_event, AddrSet(std::make_pair(addr, addr))));
	}
	else
	{
		for(auto& edge : comms)
		{
			if(std::get<0>(edge) == writer && std::get<1>(edge) == writer_event)
			{
				std::get<2>(edge).insert(std::make_pair(addr,addr));
				return;
			}
		}

		comms.push_back(std::make_tuple(writer, writer_event, AddrSet(std::make_pair(addr, addr))));
	}
}


inline void STCommEvent::reset()
{
	comms.clear();
	is_empty = true;
}


////////////////////////////////////////////////////////////
// SynchroTrace - Communication Event
////////////////////////////////////////////////////////////
STInstrEvent::STInstrEvent(const std::shared_ptr<spdlog::logger> &logger)
	: logger(logger)
{
	reset();
}


void STInstrEvent::append_instr(Addr addr)
{
	instrs << "! " << addr << " ";
	is_empty = false;
}


inline void STInstrEvent::flush()
{
	if(is_empty == false)
	{
		logger->info(instrs.str());
		reset();
	}
}


inline void STInstrEvent::reset()
{
	instrs.str(std::string());
	instrs << std::hex << std::showbase;
	is_empty = true;
}


////////////////////////////////////////////////////////////
// SynchroTrace - Synchronization Event
////////////////////////////////////////////////////////////
STSyncEvent::STSyncEvent(TId &tid, EId &eid, const std::shared_ptr<spdlog::logger> &logger)
	: thread_id(tid)
	, event_id(eid)
	, logger(logger) { }


void STSyncEvent::flush(const UChar type, const Addr sync_addr)
{
	std::stringstream logmsg;
	logmsg << event_id
		<< "," << thread_id
		<< "," << "pth_ty:"
		<< static_cast<int>(type)
		<< "^"
		<< std::hex << std::showbase
		<< sync_addr;
	logger->info(logmsg.str());
	++event_id;
}


////////////////////////////////////////////////////////////
// Unique Address Set
////////////////////////////////////////////////////////////
void AddrSet::insert(const AddrRange &range)
{
/* TODO clean up flow control */

	assert(range.first <= range.second);

	/* insert if this is the first addr */
	if(ms.empty() == true)
	{
		ms.insert(range);
		return;
	}

	/* get the first addr pair that is not less than range */
	/* see http://en.cppreference.com/w/cpp/utility/pair/operator_cmp */
	auto it = ms.lower_bound(range);

	if(it != ms.cbegin())
	{
		if(it == ms.cend())
		/* if no address range starts at a higher address, 
		 * check the last element */
		{
			it = --ms.cend();
		}
		else
		/* check if the previous addr pair overlaps with range */
		{
			--it;
			if(range.first > it->second+1)
			{
				++it;
			}
		}
	}

	if(range.first == it->second+1)
	{
		/* extend 'it' by 'range'; recheck, may overrun other addresses */
		auto tmp = std::make_pair(it->first, range.second);
		ms.erase(it);
		insert(tmp);
	}
	else if(range.second+1 == it->first)
	{
		/* extend 'it' by 'range'; recheck, may overrun other addresses */
		auto tmp = std::make_pair(range.first, it->second);
		ms.erase(it);
		insert(tmp);
	}
	else if(range.first > it->second)
	{
		/* can't merge, just insert (at end) */
		ms.insert(range);
	}
	else if(range.first >= it->first)
	{
		if(range.second > it->second)
		/* extending 'it' to the end of 'range' */
		{
			/* merge, delete, and recheck, may overrun other addresses */
			auto tmp = std::make_pair(it->first, range.second);
			ms.erase(it);
			insert(tmp);
		}
		/* else do not insert; 'it' encompasses 'range' */
	}
	else /* if (range.first < it->first) */
	{
		if(range.second < it->first)
		/* no overlap */
		{
			/* nothing to merge */
			ms.insert(range);
		}
		else if(range.second <= it->second)
		/* begin address is extended */
		{
			/* merge, delete, and insert; no need to recheck */
			Addr second = it->second;
			ms.erase(it);
			ms.emplace(range.first, second);
		}
		else /* if(range.second > it->second) */
		/* 'range' encompasses 'it' */
		{
			/* delete old range and insert bigger range; recheck */
			ms.erase(it);
			insert(range);
		}
	}
}


void AddrSet::clear()
{
	ms.clear();
}

}; //end namespace STGen

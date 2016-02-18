#include "EventHandlers.hpp"
#include <cassert>
#include "spdlog.h"

namespace STGen
{

////////////////////////////////////////////////////////////
// Synchronization Event Handling
////////////////////////////////////////////////////////////
void EventHandlers::onSyncEv(SglSyncEv ev)
{
	/* flush any outstanding ST events */
	st_comm_ev.flush();
	st_comp_ev.flush();

	if/*switching threads*/( ev.type == SyncType::SGLPRIM_SYNC_SWAP 
			&& curr_thread_id != static_cast<TId>(ev.id) )
	{
		if/*new thread*/(event_ids.find(ev.id) == event_ids.cend())
		{
			stdout_logger->info()
				<< "creating thread: " << ev.id;
		}
		setThread(ev.id);
	}
	else
	{
		UChar STtype = 0;
		switch( ev.type )
		{
		/* Convert sync type to SynchroTrace's expected value
		 * From SynchroTraceSim source code:
		 *
		 * #define P_MUTEX_LK              1
		 * #define P_MUTEX_ULK             2
		 * #define P_CREATE                3
		 * #define P_JOIN                  4
		 * #define P_BARRIER_WT            5
		 * #define P_COND_WT               6
		 * #define P_COND_SG               7
		 * #define P_SPIN_LK               8
		 * #define P_SPIN_ULK              9
		 * #define P_SEM_INIT              10
		 * #define P_SEM_WAIT              11
		 * #define P_SEM_POST              12
		 * #define P_SEM_GETV              13
		 * #define P_SEM_DEST              14
		 *
		 * NOTE: semaphores are not supported in SynchroTraceGen
		 */

		case SyncType::SGLPRIM_SYNC_LOCK:
			STtype = 1;
			break;
		case SyncType::SGLPRIM_SYNC_UNLOCK:
			STtype = 2;
			break;
		case SyncType::SGLPRIM_SYNC_CREATE:
		{
			thread_spawns.emplace(curr_thread_id, ev.id);
		}
			STtype = 3;
			break;
		case SyncType::SGLPRIM_SYNC_JOIN:
			STtype = 4;
			break;
		case SyncType::SGLPRIM_SYNC_BARRIER:
		{
			barrier_participants.emplace(ev.id, curr_thread_id);
		}
			STtype = 5;
			break;
		case SyncType::SGLPRIM_SYNC_CONDWAIT:
			STtype = 6;
			break;
		case SyncType::SGLPRIM_SYNC_CONDSIG:
			STtype = 7;
			break;
		case SyncType::SGLPRIM_SYNC_SPINLOCK:
			STtype = 8;
			break;
		case SyncType::SGLPRIM_SYNC_SPINUNLOCK:
			STtype = 9;
			break;
		default:
			/* ignore sync event */
			break;
		}

		if/*valid sync event*/( STtype > 0 )
		{
			st_sync_ev.flush(STtype, ev.id);
		}
	}
}

////////////////////////////////////////////////////////////
// Compute Event Handling
////////////////////////////////////////////////////////////
void EventHandlers::onCompEv(SglCompEv ev)
{
	/* local compute event, flush most recent comm event */
	st_comm_ev.flush(); 

	switch( ev.type )
	{
	case CompCostType::SGLPRIM_COMP_IOP:
		st_comp_ev.incIOP();
		break;
	case CompCostType::SGLPRIM_COMP_FLOP:
		st_comp_ev.incFLOP();
		break;
	default:
		break;
	}
}

////////////////////////////////////////////////////////////
// Memory Access Event Handling
////////////////////////////////////////////////////////////
void EventHandlers::onMemEv(SglMemEv ev)
{
	switch( ev.type )
	{
	case MemType::SGLPRIM_MEM_LOAD:
		onLoad(ev);
		break;
	case MemType::SGLPRIM_MEM_STORE:
		onStore(ev);
		break;
	default:
		break;
	}

	if ( st_comp_ev.store_cnt > 99 || st_comp_ev.load_cnt > 99 )
	{
		st_comp_ev.flush();
	}
}

void EventHandlers::onLoad(const SglMemEv& ev)
{
	/* incremented per event */
	st_comp_ev.load_cnt++;

	/* each byte of the read may have been touched by a different thread */
	for/*each byte*/( UInt i=0; i<ev.size; ++i )
	{
		Addr curr_addr = ev.begin_addr+i;
		TId writer_thread = shad_mem.getWriterTID(curr_addr);
		TId reader_thread = shad_mem.getReaderTID(curr_addr);

		
		if/*comm edge*/( (writer_thread != curr_thread_id) 
				&& (reader_thread != curr_thread_id)
				&& (writer_thread != SO_UNDEF )) /* FIXME treat a read/write to an address 
												   with UNDEF thread as local compute event */
		{
			st_comp_ev.flush();
			st_comm_ev.addEdge(writer_thread, shad_mem.getWriterEID(curr_addr), curr_addr);
		}
		else/*local load, comp event*/
		{
			st_comm_ev.flush();
			st_comp_ev.updateReads(curr_addr, 1);
		}
	}
}

void EventHandlers::onStore(const SglMemEv& ev)
{
	/* incremented per event */
	st_comp_ev.store_cnt++;

	st_comp_ev.updateWrites(ev);
	shad_mem.updateWriter(
			ev.begin_addr, 
			ev.size,
			curr_thread_id,
			curr_event_id);
}

////////////////////////////////////////////////////////////
// Cleanup - Flush remaining events
////////////////////////////////////////////////////////////
void EventHandlers::cleanup()
{
	st_comm_ev.flush();
	st_comp_ev.flush();
	/* sync events already flush immediately */

	stdout_logger->info("Flushing thread metadata");

	for (auto& pair : thread_spawns)
	{
		/* XXX ML: is this always going to be posix threads? */
		stdout_logger->info()
			<< "pthread create from thread_id:" << pair.first 
			<< " -- pthread_t:" << pair.second;
	}

	for (auto& pair : barrier_participants)
	{
		stdout_logger->info()
			<< "barrier:" << pair.first 
			<< " -- thread:" << pair.second;
	}

	stdout_logger->flush();
	if (curr_logger != nullptr) curr_logger->flush();
}


////////////////////////////////////////////////////////////
// Miscellaneous
////////////////////////////////////////////////////////////
namespace
{
std::map<std::string,std::string> ANSIcolors_fg =
{
	{"black", "\033[30m"},
	{"red", "\033[31m"},
	{"green", "\033[32m"},
	{"yellow", "\033[33m"},
	{"blue", "\033[34m"},
	{"magenta", "\033[35m"},
	{"cyan", "\033[36m"},
	{"white", "\033[37m"},
	{"end", "\033[0m"}
};
}; //end namespace 

constexpr const char EventHandlers::filename[];
EventHandlers::EventHandlers()
	: st_comp_ev(curr_thread_id, curr_event_id, curr_logger)
	, st_comm_ev(curr_thread_id, curr_event_id, curr_logger)
	, st_sync_ev(curr_thread_id, curr_event_id, curr_logger)
{
	loggers.resize(16, nullptr);
	stdout_logger = spdlog::stdout_logger_st("stgen-console");

	std::string header = "[SynchroTraceGen]";
	if (isatty(fileno(stdout))) header = "[" + ANSIcolors_fg["blue"] + "SynchroTraceGen" + ANSIcolors_fg["end"] + "]";

	stdout_logger->set_pattern(header+" %v");

	curr_thread_id = -1;
	curr_event_id = -1;
}

void EventHandlers::setThread(TId tid)
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

void EventHandlers::initThreadLog(TId tid)
{
	assert( tid >= 0 );

	std::string thread_filename = filename + std::to_string(tid);

	spdlog::set_async_mode(8192);
	spdlog::create_simple_file_logger(std::to_string(tid), thread_filename, true);
	spdlog::get(std::to_string(tid))->set_pattern("%v");

	if ( static_cast<int>(loggers.size()) <= tid )
	{
		loggers.resize(loggers.size()*2, nullptr);
	}
	loggers[tid] = spdlog::get(std::to_string(tid));

	curr_logger = loggers[tid];
}

void EventHandlers::switchThreadLog(TId tid)
{
	assert( static_cast<int>(loggers.size()) > tid );
	assert( loggers[tid] != nullptr );

	curr_logger = loggers[tid];
}

////////////////////////////////////////////////////////////
// Callbacks
////////////////////////////////////////////////////////////
namespace
{
EventHandlers handler;
};

void onSyncEv(SglSyncEv ev)
{
	handler.onSyncEv(ev);
}

void onCompEv(SglCompEv ev)
{
	handler.onCompEv(ev);
}

void onMemEv(SglMemEv ev)
{
	handler.onMemEv(ev);
}

void cleanup()
{
	handler.cleanup();
}


}; //end namespace STGen

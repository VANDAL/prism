#include "EventHandlers.hpp"
#include <cassert>
#include "spdlog.h"

namespace STGen
{
extern std::shared_ptr<spdlog::logger> curr_logger;

EventHandlers::EventHandlers()
{
	/* to initialize loggers;
	 * FIXME let front end initialize this
	 * Thread ID #0 is assumed as first thread from Sigrind */
	STEvent::setThread(0);
}

////////////////////////////////////////////////////////////
// Synchronization Event Handling
////////////////////////////////////////////////////////////
void EventHandlers::onSyncEv(SglSyncEv ev)
{
	switch( ev.type )
	{
	case SyncType::SGLPRIM_SYNC_SWAP:
		if ( STEvent::curr_thread_id != static_cast<TId>(ev.id) )
		{
			st_comm_ev.flush();
			st_comp_ev.flush();

			STEvent::setThread(ev.id);
		}
		break;
	case SyncType::SGLPRIM_SYNC_CREATE:
	case SyncType::SGLPRIM_SYNC_JOIN:
	case SyncType::SGLPRIM_SYNC_BARRIER:
	case SyncType::SGLPRIM_SYNC_SYNC:
	case SyncType::SGLPRIM_SYNC_LOCK:
	case SyncType::SGLPRIM_SYNC_UNLOCK:
	case SyncType::SGLPRIM_SYNC_CONDWAIT:
	case SyncType::SGLPRIM_SYNC_CONDSIG:
	case SyncType::SGLPRIM_SYNC_SPINLOCK:
	case SyncType::SGLPRIM_SYNC_SPINUNLOCK:
		st_sync_ev.logSync(ev.type, ev.id);
		break;
	default:
		assert(0); //TODO will this validly happen?
		break;
	}
}

////////////////////////////////////////////////////////////
// Compute Event Handling
////////////////////////////////////////////////////////////
void EventHandlers::onCompEv(SglCompEv ev)
{
	st_comm_ev.flush(); /* local compute event, 
						   flush most recent communication event */
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
	//Each byte of the read may have been touched by a different thread
	for/*each byte*/( UInt i=0; i<ev.size; ++i )
	{
		Addr curr_addr = ev.begin_addr+i;
		TId writer_thread = shad_mem.getWriterTID(curr_addr);
		TId reader_thread = shad_mem.getReaderTID(curr_addr);

		
		if/*comm edge*/( (writer_thread != STEvent::curr_thread_id) 
				&& (reader_thread != STEvent::curr_thread_id)
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
	st_comp_ev.updateWrites(ev);
	shad_mem.updateWriter(
			ev.begin_addr, 
			ev.size,
			STEvent::curr_thread_id,
			STEvent::curr_event_id);
}

////////////////////////////////////////////////////////////
// Cleanup - Flush remaining events
////////////////////////////////////////////////////////////
void EventHandlers::cleanup()
{
	st_comm_ev.flush();
	st_comp_ev.flush();
	curr_logger->flush();
}

}; //end namespace STGen

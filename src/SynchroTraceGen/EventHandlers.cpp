#include "EventHandlers.hpp"
#include <cassert>

namespace STGen
{
EventHandlers::EventHandlers()
{
	/* to initialize loggers;
	 * assume first thread is id 0 */
	STEvent::setThread(0);
}

EventHandlers::~EventHandlers()
{
}

////////////////////////////////////////////////////////////
// Synchronization Event Handling
////////////////////////////////////////////////////////////
void EventHandlers::onSyncEv(SglSyncEv ev)
{
	switch( ev.type )
	{
		case SyncType::SYNC_SWAP:
			if ( STEvent::curr_thread_id != static_cast<TId>(ev.id) )
			{
				st_comp_ev.flush();
				st_comm_ev.flush();

				STEvent::setThread(ev.id);
			}
			break;
		case SyncType::SYNC_CREATE:
		case SyncType::SYNC_JOIN:
		case SyncType::SYNC_BARRIER:
		case SyncType::SYNC_SYNC:
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
		case CompCostType::COMP_IOP:
			st_comp_ev.incIOP();
			break;
		case CompCostType::COMP_FLOP:
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
		case MemType::MEM_LOAD:
			onLoad(ev);
			break;
		case MemType::MEM_STORE:
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

void EventHandlers::onLoad(const SglMemEv& ev_data)
{
	//Each byte of the read may have been touched by a different thread
	for/*each byte*/( UInt i=0; i<ev_data.size; ++i )
	{
		Addr curr_addr = ev_data.begin_addr+i;
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

void EventHandlers::onStore(const SglMemEv& ev_data)
{
	st_comp_ev.updateWrites(ev_data);
	shad_mem.updateWriter(
			ev_data.begin_addr, 
			ev_data.size,
			STEvent::curr_thread_id,
			STEvent::curr_event_id);
}

////////////////////////////////////////////////////////////
// Context Switch Event Handling
////////////////////////////////////////////////////////////
void EventHandlers::onCxtEv(SglCxtEv ev)
{
	//nothing to do
}
}; //end namespace STGen

#ifndef STGEN_EVENTHANDLERS_H
#define STGEN_EVENTHANDLERS_H

#include "ShadowMemory.hpp"
#include "LogHelpers.hpp"
#include "STEvent.hpp"

namespace STGen
{
class EventHandlers
{
	typedef int32_t TId; 
public:
	EventHandlers();

	void onSyncEv(SglSyncEv ev);
	void onCompEv(SglCompEv ev);
	void onMemEv(SglMemEv ev);
	void cleanup();

	/* SynchroTraceGen makes use of 3 SynchroTrace events,
	 * i.e. Computation, Communication, and Synchronization.
	 *
	 * One of each event is populated and flushed as Sigil
	 * primitives are processed. Because there might be billions
	 * or more of SynchroTrace events, dynamic heap allocation of 
	 * consecutive SynchroTrace events is avoided */
	STCompEvent st_comp_ev;
	STCommEvent st_comm_ev;
	STSyncEvent st_sync_ev;

private:
	void onLoad(const SglMemEv& ev_data);
	void onStore(const SglMemEv& ev_data);

	ShadowMemory shad_mem;
};

}; //end namespace STGen

#endif

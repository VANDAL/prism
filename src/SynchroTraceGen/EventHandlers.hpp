#ifndef STGEN_EVENTHANDLERS_H
#define STGEN_EVENTHANDLERS_H

#include <unistd.h>
#include <memory>
#include <fstream>
#include "spdlog.h"
#include "ozstream.hpp"
#include "ShadowMemory.hpp"
#include "STEvent.hpp"

namespace STGen
{
using std::make_shared;
using std::shared_ptr;

void onCompEv(SglCompEv ev);
void onMemEv(SglMemEv ev);
void onSyncEv(SglSyncEv ev);
void cleanup();

class EventHandlers
{
	/* Default address params */
	ShadowMemory shad_mem;

	/* Per-thread event count. Logged to SynchroTrace event trace.
	 * Each derived SynchroTrace event tracks the same event id.  */
	std::unordered_map<TId, EId> event_ids;
	TId curr_thread_id;
	EId curr_event_id;

	/* spawner, address of spawnee thread_t */
	std::multimap<TId, Addr> thread_spawns;

	/* addr of barrier_t, participating thread */
	std::multimap<Addr, TId> barrier_participants;

	/* Compatibility with SynchroTraceSim parser */ 
	constexpr const static char filename[18] = "sigil.events.out-";

	/* Output directly to a *.gz stream to save space */
	/* Keep these ostreams open until deconstruction */
	vector<shared_ptr<std::ofstream>> gzlog_files;
	vector<shared_ptr<zstream::ogzstream>> gzlog_streams;
	vector<shared_ptr<spdlog::logger>> loggers;

	/* One logger for stdout,
	 * one logger for the current thread event log */
	shared_ptr<spdlog::logger> curr_logger;
	shared_ptr<spdlog::logger> stdout_logger;

public:
	EventHandlers();
	EventHandlers(const EventHandlers&) = delete;
	EventHandlers& operator=(const EventHandlers&) = delete;
	~EventHandlers();

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
	void setThread(TId tid);
	void initThreadLog(TId tid);
	void switchThreadLog(TId tid);
};

}; //end namespace STGen

#endif

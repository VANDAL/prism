#ifndef STGEN_EVENTHANDLERS_H
#define STGEN_EVENTHANDLERS_H

#include <unistd.h>
#include <memory>
#include <fstream>
#include "spdlog.h"
#include "zfstream.h"
#include "ShadowMemory.hpp"
#include "STEvent.hpp"

namespace STGen
{
using namespace std;

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
	unordered_map<TId, EId> event_ids;
	TId curr_thread_id;
	EId curr_event_id;

	/* vector of spawner, address of spawnee thread_t
	 *
	 * All addresses associated with the same spawner are in
	 * the order they were inserted */
	vector<pair<TId, Addr>> thread_spawns;

	/* each spawned thread's ID, in the order it first seen ('created') */
	vector<TId> thread_creates;

	/* vecotr of barrier_t addr, participating threads
	 *
	 * order matters for SynchroTraceSim */
	vector<pair<Addr, set<TId>>> barrier_participants;

	/* Output directly to a *.gz stream to save space */
	/* Keep these ostreams open until deconstruction */
	vector<shared_ptr<gzofstream>> gz_streams;
	map<string, shared_ptr<spdlog::logger>> loggers;

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

	/* Compatibility with SynchroTraceSim parser */ 
	constexpr const static char filebase[18] = "sigil.events.out-";

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

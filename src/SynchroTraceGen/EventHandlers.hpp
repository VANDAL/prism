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
void onSyncEv(SglSyncEv ev);
void onCompEv(SglCompEv ev);
void onMemEv(SglMemEv ev);
void cleanup();
void parseArgs(std::vector<std::string>);

class EventHandlers
{
	/* Default address params */
	ShadowMemory shad_mem;

	/* Per-thread event count. Logged to SynchroTrace event trace.
	 * Each derived SynchroTrace event tracks the same event id.  */
	std::unordered_map<TId, EId> event_ids;
	TId curr_thread_id;
	EId curr_event_id;

	/* vector of spawner, address of spawnee thread_t
	 *
	 * All addresses associated with the same spawner are in
	 * the order they were inserted */
	std::vector<std::pair<TId, Addr>> thread_spawns;

	/* each spawned thread's ID, in the order it first seen ('created') */
	std::vector<TId> thread_creates;

	/* vecotr of barrier_t addr, participating threads
	 *
	 * order matters for SynchroTraceSim */
	std::vector<std::pair<Addr, std::set<TId>>> barrier_participants;

	/* Output directly to a *.gz stream to save space */
	/* Keep these ostreams open until deconstruction */
	std::vector<std::shared_ptr<gzofstream>> gz_streams;
	std::map<std::string, std::shared_ptr<spdlog::logger>> loggers;
	std::shared_ptr<spdlog::logger> curr_logger;

	std::string filename(TId tid)
	{
		return output_directory + "/" + filebase +
			std::to_string(tid) + ".gz";
	}

public:
	EventHandlers();
	EventHandlers(const EventHandlers&) = delete;
	EventHandlers& operator=(const EventHandlers&) = delete;
	~EventHandlers();

	/* Compatibility with SynchroTraceSim parser */ 
	const char filebase[32] = "sigil.events.out-";
	std::string output_directory;

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

#include <cassert>
#include <sstream>

#include "spdlog/sinks/ostream_sink.h"
#include "Sigil2/SigiLog.hpp"

#include "EventHandlers.hpp"

namespace STGen
{

////////////////////////////////////////////////////////////
// Shared state
////////////////////////////////////////////////////////////
/* all instances share the same shadow memory state */
STShadowMemory EventHandlers::shad_mem;

std::string EventHandlers::output_directory{"."};
const std::string EventHandlers::filebase = "sigil.events.out-";
unsigned int EventHandlers::primitives_per_st_comp_ev{100};


////////////////////////////////////////////////////////////
// Local stats for each thread
// Required for CPI estimates in SynchroTrace
////////////////////////////////////////////////////////////
decltype(PerThreadStats::per_thread_counts) PerThreadStats::per_thread_counts;
decltype(PerThreadStats::per_thread_mutex) PerThreadStats::per_thread_mutex;

void PerThreadStats::sync()
{
    if(curr_tid != INVL_TID)
    {
        std::lock_guard<std::mutex> lock(per_thread_mutex);
        per_thread_counts[curr_tid].first = iop_count;
        per_thread_counts[curr_tid].second = flop_count;
    }
}

bool PerThreadStats::isNewThread(TID tid)
{
    std::lock_guard<std::mutex> lock(per_thread_mutex);
    return per_thread_counts.find(tid) == per_thread_counts.cend();
}

void PerThreadStats::setThread(TID tid)
{
    /* XXX MDL20161018 Performance Concern:
     * Up to 3 mutex locks occur in this function.
     * If threads switch often enough,
     * this could become a significant bottleneck. */

    sync();

    if(isNewThread(tid))
    {
        iop_count = 0;
        flop_count = 0;
    }
    else
    {
        std::lock_guard<std::mutex> lock(per_thread_mutex);
        iop_count = per_thread_counts[tid].first;
        flop_count = per_thread_counts[tid].second;
    }

    curr_tid = tid;
}

StatCounter PerThreadStats::getThreadIOPS(TID tid)
{
    sync();
    std::lock_guard<std::mutex> lock(per_thread_mutex);
    return per_thread_counts[tid].first;
}

StatCounter PerThreadStats::getThreadFLOPS(TID tid)
{
    sync();
    std::lock_guard<std::mutex> lock(per_thread_mutex);
    return per_thread_counts[tid].second;
}


////////////////////////////////////////////////////////////
// Synchronization Event Handling
////////////////////////////////////////////////////////////
std::mutex sync_event_mutex;
std::vector<std::pair<TID, Addr>> thread_spawns;
std::vector<TID> thread_creates;
std::vector<std::pair<Addr, std::set<TID>>> barrier_participants;

void EventHandlers::onSyncEv(const SglSyncEv &ev)
{
    /* Flush any outstanding ST events */
    st_comm_ev.flush();
    st_comp_ev.flush();

    if /*switching threads*/(ev.type == SyncTypeEnum::SGLPRIM_SYNC_SWAP &&
                             curr_thread_id != static_cast<TID>(ev.id))
    {
        if /*new thread*/(event_ids.find(ev.id) == event_ids.cend())
        {
            std::lock_guard<std::mutex> lock(sync_event_mutex);
            thread_creates.push_back(ev.id);
        }

        setThread(ev.id);
    }
    else
    {
        SyncType type = 0;

        switch (ev.type)
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
         * #define P_COND_BROAD            8
         * #define P_SPIN_LK               9
         * #define P_SPIN_ULK              10
         * #define P_SEM_INIT              11
         * #define P_SEM_WAIT              12
         * #define P_SEM_POST              13
         * #define P_SEM_GETV              14
         * #define P_SEM_DEST              15
         *
         * NOTE: semaphores are not supported in SynchroTraceGen
         */

        case ::SGLPRIM_SYNC_LOCK:
            type = 1;
            break;

        case ::SGLPRIM_SYNC_UNLOCK:
            type = 2;
            break;

        case ::SGLPRIM_SYNC_CREATE:
        {
            std::lock_guard<std::mutex> lock(sync_event_mutex);
            thread_spawns.push_back(std::make_pair(curr_thread_id, ev.id));
        }

        type = 3;
        break;

        case ::SGLPRIM_SYNC_JOIN:
            type = 4;
            break;

        case ::SGLPRIM_SYNC_BARRIER:
        {
            std::lock_guard<std::mutex> lock(sync_event_mutex);

            unsigned int idx = 0;

            for (auto &pair : barrier_participants)
            {
                if (pair.first == (unsigned long)ev.id)
                {
                    break;
                }

                ++idx;
            }

            if /*no matches found*/(idx == barrier_participants.size())
            {
                barrier_participants.push_back(make_pair(ev.id, std::set<TID>({curr_thread_id})));
            }
            else
            {
                barrier_participants[idx].second.insert(curr_thread_id);
            }
        }

        type = 5;
        break;

        case ::SGLPRIM_SYNC_CONDWAIT:
            type = 6;
            break;

        case ::SGLPRIM_SYNC_CONDSIG:
            type = 7;
            break;

        case ::SGLPRIM_SYNC_CONDBROAD:
            type = 8;
            break;

        case ::SGLPRIM_SYNC_SPINLOCK:
            type = 9;
            break;

        case ::SGLPRIM_SYNC_SPINUNLOCK:
            type = 10;
            break;

        default:
            /* Ignore sync event */
            break;
        }

        if /*valid sync event*/(type > 0)
        {
            st_sync_ev.flush(type, ev.id);
        }
    }
}


////////////////////////////////////////////////////////////
// Compute Event Handling
////////////////////////////////////////////////////////////
void EventHandlers::onCompEv(const SglCompEv &ev)
{
    /* Local compute event, flush most recent comm event */
    st_comm_ev.flush();

    switch (ev.type)
    {
    case CompCostTypeEnum::SGLPRIM_COMP_IOP:
        ++stats.iop_count;
        st_comp_ev.incIOP();
        break;

    case CompCostTypeEnum::SGLPRIM_COMP_FLOP:
        ++stats.flop_count;
        st_comp_ev.incFLOP();
        break;

    default:
        break;
    }
}


////////////////////////////////////////////////////////////
// Memory Access Event Handling
////////////////////////////////////////////////////////////
void EventHandlers::onMemEv(const SglMemEv &ev)
{
    switch (ev.type)
    {
    case MemTypeEnum::SGLPRIM_MEM_LOAD:
        onLoad(ev);
        break;

    case MemTypeEnum::SGLPRIM_MEM_STORE:
        onStore(ev);
        break;

    default:
        break;
    }

    if (st_comp_ev.thread_local_store_cnt > (primitives_per_st_comp_ev - 1) ||
        st_comp_ev.thread_local_load_cnt > (primitives_per_st_comp_ev - 1))
    {
        st_comp_ev.flush();
    }
}


void EventHandlers::onLoad(const SglMemEv &ev)
{
    bool is_comm_edge = false;

    /* Each byte of the read may have been touched by a different thread */
    for /*each byte*/(decltype(ev.size) i = 0; i < ev.size; ++i)
    {
        Addr curr_addr = ev.begin_addr + i;
        TID writer_thread = shad_mem.getWriterTID(curr_addr);
        bool is_reader_thread = shad_mem.isReaderTID(curr_addr, curr_thread_id);

        if (is_reader_thread == false)
        {
            shad_mem.updateReader(curr_addr, 1, curr_thread_id);
        }

        if /*comm edge*/((is_reader_thread == false) &&
                         (writer_thread != curr_thread_id) &&
                         (writer_thread != SO_UNDEF)) /* XXX treat a read/write
                                                       * to an address with UNDEF thread
                                                       * as a local compute event */
        {
            is_comm_edge = true;
            st_comp_ev.flush();
            st_comm_ev.addEdge(writer_thread, shad_mem.getWriterEID(curr_addr), curr_addr);
        }
        else/*local load, comp event*/
        {
            st_comm_ev.flush();
            st_comp_ev.updateReads(curr_addr, 1);
        }

    }

    /* A situation when a singular memory event is both
     * a communication edge and a local thread read is
     * rare and not robustly accounted for. A single address
     * that is a communication edge counts the whole event as
     * a communication event, and not as part of a
     * computation event */
    if (is_comm_edge == false)
    {
        st_comp_ev.incReads();
    }
}


void EventHandlers::onStore(const SglMemEv &ev)
{
    st_comp_ev.incWrites();
    st_comp_ev.updateWrites(ev);

    shad_mem.updateWriter(ev.begin_addr, ev.size, curr_thread_id, curr_event_id);
}


////////////////////////////////////////////////////////////
// Context Event Handling (instructions)
////////////////////////////////////////////////////////////
void EventHandlers::onCxtEv(const SglCxtEv &ev)
{
    /* Instruction address marker */
    if (ev.type == CxtTypeEnum::SGLPRIM_CXT_INSTR)
    {
        st_cxt_ev.append_instr(ev.id);
    }
}


////////////////////////////////////////////////////////////
// Flush any pthread data
////////////////////////////////////////////////////////////

/* XXX MDL20161018 should only be invoked after all instances of EventHandlers are
 * destroyed. This is the expected case and the user should not have to
 * do anything special */
void onExit()
{
    std::string pthread_metadata(EventHandlers::output_directory + "/sigil.pthread.out");
    std::ofstream pthread_file(pthread_metadata, std::ios::trunc | std::ios::out);

    if (pthread_file.fail() == true)
    {
        SigiLog::fatal("Failed to open: " + pthread_metadata);
    }

    spdlog::set_sync_mode();
    auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_st>(pthread_file);
    auto pthread_logger = spdlog::create(pthread_metadata, {ostream_sink});
    pthread_logger->set_pattern("%v");

    SigiLog::info("Flushing thread metadata to: " + pthread_metadata);

    /* The order the threads were seen SHOULD match to
     * the order of thread_t values of the pthread_create
     * calls. For example, with the valgrind frontend,
     * the --fair-sched=yes option should make sure each
     * thread is switched to in the order they were created */
    assert(thread_spawns.size() == thread_creates.size());
    int create_idx = 1; //skip the first idx, which is the initial thread

    for (auto &pair : thread_spawns)
    {
        /* SynchroTraceSim only supports threads
         * that were spawned from the original thread */
        if (pair.first == 1)
        {
            pthread_logger->info("##" + std::to_string(pair.second) +
                                 "," + std::to_string(thread_creates[create_idx]));
        }

        ++create_idx; //Skip past thread spawns that happened in other threads
    }

    /* TODO Confirm with KS and SN how barriers are processed */
    /* Iterate through each unique barrier_t address and
     * aggregate all the associated, participating threads */
    for (auto &pair : barrier_participants)
    {
        std::ostringstream ss;
        ss << "**" << pair.first;

        for (auto &tid : pair.second)
        {
            ss << "," << tid;
        }

        pthread_logger->info(ss.str());
    }

    /* For SynchroTraceSim CPI calculations */
    pthread_logger->info("Total instructions: " + std::to_string(STInstrEvent::instr_count));

    for (auto &p : PerThreadStats::per_thread_counts)
    {
        pthread_logger->info("Thread Stats: " + std::to_string(p.first));
        pthread_logger->info("\tIOPS : " + std::to_string(p.second.first));
        pthread_logger->info("\tFLOPS: " + std::to_string(p.second.second));
    }

    pthread_logger->flush();
}


////////////////////////////////////////////////////////////
// Miscellaneous
////////////////////////////////////////////////////////////
EventHandlers::EventHandlers()
    : st_cxt_ev(curr_logger)
    , st_comp_ev(curr_thread_id, curr_event_id, curr_logger, st_cxt_ev)
    , st_comm_ev(curr_thread_id, curr_event_id, curr_logger, st_cxt_ev)
    , st_sync_ev(curr_thread_id, curr_event_id, curr_logger)
{
    curr_thread_id = SO_UNDEF;
    curr_event_id = 0;
}


EventHandlers::~EventHandlers()
{
    st_comm_ev.flush();
    st_comp_ev.flush();
    // sync events already flush immediately

    /* close remaining logs before gzstreams close
     * to prevent nasty race conditions that can
     * manifest if asynchronous logging is enabled
     *
     * the destructors should call a blocking flush */
    for (auto &p : loggers)
    {
        p.second->flush();
        p.second.reset();
        spdlog::drop(std::to_string(p.first));
    }

    curr_logger.reset();

    /* close streams */
    for (auto &ptr : gz_streams)
    {
        ptr.reset();
    }
}


void EventHandlers::setThread(TID tid)
{
    assert(tid >= 0);

    stats.setThread(tid);

    if (curr_thread_id == tid)
    {
        return;
    }

    event_ids[curr_thread_id] = curr_event_id;

    if /*new thread*/(event_ids.find(tid) == event_ids.cend())
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


void EventHandlers::initThreadLog(TID tid)
{
    assert(tid >= 0);

    /* make the filename == logger key */
    auto key = filename(tid);

    auto thread_gz = std::make_shared<gzofstream>(key.c_str(), std::ios::trunc | std::ios::out);

    if (thread_gz->fail() == true)
    {
        SigiLog::fatal("Failed to open: " + key);
    }

    auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_st>(*thread_gz);

    spdlog::set_async_mode(1 << 10);

    curr_logger = spdlog::create(std::to_string(tid), {ostream_sink});
    curr_logger->set_pattern("%v");
    loggers[tid] = curr_logger;

    /* keep ostreams alive */
    gz_streams.push_back(thread_gz);
}


void EventHandlers::switchThreadLog(TID tid)
{
    assert(loggers.find(tid) != loggers.cend());

    curr_logger->flush();
    curr_logger = loggers[tid];
}


////////////////////////////////////////////////////////////
// Option Parsing
////////////////////////////////////////////////////////////
void onParse(Sigil::Args args)
{
    /* only accept short options */
    std::set<char> options;
    std::map<char, std::string> matches;

    options.insert('o'); // -o OUTPUT_DIRECTORY
    options.insert('c'); // -c COMPRESSION_VALUE

    int unmatched = 0;

    for (auto arg = args.cbegin(); arg != args.cend(); ++arg)
    {
        if /*opt found, '-<char>'*/(((*arg).substr(0, 1).compare("-") == 0) &&
                                    ((*arg).length() > 1))
        {
            /* check if the option matches */
            auto opt = options.cbegin();

            for (; opt != options.cend(); ++opt)
            {
                if ((*arg).substr(1, 1).compare({*opt}) == 0)
                {
                    if ((*arg).length() > 2)
                    {
                        matches[*opt] = (*arg).substr(2, std::string::npos);
                    }
                    else if (arg + 1 != args.cend())
                    {
                        matches[*opt] = *(++arg);
                    }

                    break;
                }
            }

            if /*no match*/(opt == options.cend())
            {
                ++unmatched;
            }
        }
        else
        {
            ++unmatched;
        }
    }

    if (unmatched > 0)
    {
        SigiLog::fatal("unexpected synchrotracegen options");
    }

    if (matches['o'].empty() == false)
    {
        EventHandlers::output_directory = matches['o'];
    }

    if (matches['c'].empty() == false)
    {
        try
        {
            EventHandlers::primitives_per_st_comp_ev = std::stoi(matches['c']);
        }
        catch (std::invalid_argument &e)
        {
            SigiLog::fatal(std::string("SynchroTraceGen compression level: invalid argument"));
        }
        catch (std::out_of_range &e)
        {
            SigiLog::fatal(std::string("SynchroTraceGen compression level: out_of_range"));
        }
        catch (std::exception &e)
        {
            SigiLog::fatal(std::string("SynchroTraceGen compression level: ").append(e.what()));
        }
    }
}

}; //end namespace STGen

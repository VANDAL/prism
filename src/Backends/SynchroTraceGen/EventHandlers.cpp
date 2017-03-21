#include "EventHandlers.hpp"
#include "STTypes.hpp"
#include "TextLogger.hpp"
#include <cassert>

using namespace SigiLog; // console logging
namespace STGen
{

STShadowMemory ThreadContext::shadow;

template <typename T>
struct FunctionSignature;

template <typename R, typename... Args>
struct FunctionSignature<R (Args...)>
{
    using type = R(Args...);
};

template <class TCxtType>
auto ThreadContextGenerator(TID tid,
                            unsigned primsPerStCompEv,
                            std::string outputPath,
                            std::string loggerType) -> std::unique_ptr<ThreadContext>
{
    return std::unique_ptr<ThreadContext>(new TCxtType{tid,
                                                       primsPerStCompEv,
                                                       outputPath,
                                                       loggerType});
}
using TCxtGenerator = std::function<decltype(ThreadContextGenerator<ThreadContextCompressed>)>;

/* Global to all threads */
namespace
{
std::string outputPath{"."};
unsigned primsPerStCompEv{100};
std::string loggerType;
TCxtGenerator genTCxt;

std::mutex gMtx;
ThreadStatMap allThreadsStats;
SpawnList threadSpawns;
ThreadList newThreadsInOrder;
BarrierList barrierParticipants;
}; //end namespace


////////////////////////////////////////////////////////////
// Synchronization Event Handling
////////////////////////////////////////////////////////////
auto EventHandlers::onSyncEv(const SglSyncEv &ev) -> void
{
    auto syncType = ev.type;
    auto syncID = ev.id;

    /* Update global state */
    if (syncType == SyncTypeEnum::SGLPRIM_SYNC_SWAP)
        return onSwapTCxt(syncID);
    else if (syncType == SyncTypeEnum::SGLPRIM_SYNC_CREATE)
        onCreate(syncID);
    else if (syncType == SyncTypeEnum::SGLPRIM_SYNC_BARRIER)
        onBarrier(syncID);

    convertAndFlush(syncType, syncID);
}


////////////////////////////////////////////////////////////
// Compute Event Handling
////////////////////////////////////////////////////////////
auto EventHandlers::onCompEv(const SglCompEv &ev) -> void
{
    switch (ev.type)
    {
    case CompCostTypeEnum::SGLPRIM_COMP_IOP:
        cachedTCxt->onIop();
        break;
    case CompCostTypeEnum::SGLPRIM_COMP_FLOP:
        cachedTCxt->onFlop();
        break;
    default:
        break;
    }
}


////////////////////////////////////////////////////////////
// Memory Event Handling
////////////////////////////////////////////////////////////
auto EventHandlers::onMemEv(const SglMemEv &ev) -> void
{
    switch (ev.type)
    {
    case MemTypeEnum::SGLPRIM_MEM_LOAD:
        cachedTCxt->onRead(ev.begin_addr, ev.size);
        break;
    case MemTypeEnum::SGLPRIM_MEM_STORE:
        cachedTCxt->onWrite(ev.begin_addr, ev.size);
        break;
    default:
        break;
    }
}


////////////////////////////////////////////////////////////
// Context Event Handling (instructions)
////////////////////////////////////////////////////////////
auto EventHandlers::onCxtEv(const SglCxtEv &ev) -> void
{
    if (ev.type == CxtTypeEnum::SGLPRIM_CXT_INSTR)
        cachedTCxt->onInstr();
}


////////////////////////////////////////////////////////////
// Flush final stats and data
////////////////////////////////////////////////////////////
EventHandlers::~EventHandlers()
{
    std::lock_guard<std::mutex> lock(gMtx);
    for (auto& p : tcxts)
        allThreadsStats.emplace(p.first, p.second->getStats());
}


auto onExit() -> void
{
    std::lock_guard<std::mutex> lock(gMtx);
    spdlog::set_sync_mode();
    flushPthread(outputPath + "/sigil.pthread.out", newThreadsInOrder,
                 threadSpawns, barrierParticipants);
    flushStats(outputPath + "/sigil.stats.out", allThreadsStats);
}


////////////////////////////////////////////////////////////
// Synchronization Event Helpers
////////////////////////////////////////////////////////////
auto EventHandlers::onSwapTCxt(TID newTID) -> void
{
    assert(newTID > 0);

    if (currentTID != newTID)
    {
        std::lock_guard<std::mutex> lock(gMtx);
        if (std::find(newThreadsInOrder.cbegin(),
                      newThreadsInOrder.cend(),
                      newTID) == newThreadsInOrder.cend())
        {
            newThreadsInOrder.push_back(newTID);
            tcxts.emplace(std::piecewise_construct,
                          std::forward_as_tuple(newTID),
                          std::forward_as_tuple(genTCxt(newTID, primsPerStCompEv,
                                                        outputPath, loggerType)));
        }

        if (cachedTCxt != nullptr)
            cachedTCxt->flushAll();

        currentTID = newTID;
        assert(tcxts.find(currentTID) != tcxts.cend());
        cachedTCxt = tcxts.at(currentTID).get();
    }

    assert(currentTID = newTID);
    assert(cachedTCxt != nullptr);
}

auto EventHandlers::onCreate(Addr data) -> void
{
    std::lock_guard<std::mutex> lock(gMtx);
    threadSpawns.push_back(std::make_pair(currentTID, data));
}

auto EventHandlers::onBarrier(Addr data) -> void
{
    std::lock_guard<std::mutex> lock(gMtx);

    unsigned idx = 0;
    for (auto &p : barrierParticipants)
    {
        if (p.first == data)
            break;
        ++idx;
    }

    if (idx == barrierParticipants.size())
        barrierParticipants.push_back(std::make_pair(data, std::set<TID>{currentTID}));
    else
        barrierParticipants[idx].second.insert(currentTID);
}

auto EventHandlers::convertAndFlush(SyncType type, Addr data) -> void
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
    SyncType stSyncType = 0;
    switch (type)
    {
    case ::SGLPRIM_SYNC_LOCK:
        stSyncType = 1;
        break;
    case ::SGLPRIM_SYNC_UNLOCK:
        stSyncType = 2;
        break;
    case ::SGLPRIM_SYNC_CREATE:
        stSyncType = 3;
        break;
    case ::SGLPRIM_SYNC_JOIN:
        stSyncType = 4;
        break;
    case ::SGLPRIM_SYNC_BARRIER:
        stSyncType = 5;
        break;
    case ::SGLPRIM_SYNC_CONDWAIT:
        stSyncType = 6;
        break;
    case ::SGLPRIM_SYNC_CONDSIG:
        stSyncType = 7;
        break;
    case ::SGLPRIM_SYNC_CONDBROAD:
        stSyncType = 8;
        break;
    case ::SGLPRIM_SYNC_SPINLOCK:
        stSyncType = 9;
        break;
    case ::SGLPRIM_SYNC_SPINUNLOCK:
        stSyncType = 10;
        break;
    default:
        break;
    }

    if (stSyncType > 0)
        cachedTCxt->onSync(stSyncType, data);
}


////////////////////////////////////////////////////////////
// Option Parsing
////////////////////////////////////////////////////////////
auto parseAll(const Args &args, const std::set<char> &options) -> std::map<char, std::string>
{
    std::map<char, std::string> matches;
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
                ++unmatched;
        }
        else
        {
            ++unmatched;
        }
    }

    if (unmatched > 0)
        fatal("unexpected synchrotracegen options");

    return matches;
}


auto parseLogger(std::string loggerArg) -> std::string
{
    if (loggerArg.empty() == true)
        return "text";

    std::transform(loggerArg.begin(), loggerArg.end(), loggerArg.begin(), ::tolower);
    if (loggerArg != "text" &&
        loggerArg != "capnp" &&
        loggerArg != "null")
        fatal("unexpected synchrotracegen options: -l " + loggerArg);

    return loggerArg;
}


auto parseCompression(std::string compression) -> int
{
    if (compression.empty() == true)
        return 100; // default

    try
    {
        int ret = std::stoi(compression);
        if (ret < 1)
            fatal("SynchroTraceGen compression level: invalid argument");
        return ret;
    }
    catch (std::invalid_argument &e)
    {
        fatal("SynchroTraceGen compression level: invalid argument");
    }
    catch (std::out_of_range &e)
    {
        fatal("SynchroTraceGen compression level: out_of_range");
    }
    catch (std::exception &e)
    {
        fatal(std::string("SynchroTraceGen compression level: ").append(e.what()));
    }
}


auto parseOutputPath(std::string outputPath) -> std::string
{
    if (outputPath.empty() == true)
        return "."; //default
    else
        return outputPath;
}


auto onParse(Args args) -> void
{
    /* only accept short options */
    std::set<char> options;
    options.insert('o'); // -o OUTPUT_DIRECTORY
    options.insert('c'); // -c COMPRESSION_VALUE
    options.insert('l'); // -l {text,capnp}
    auto matches = parseAll(args, options);

    outputPath = parseOutputPath(matches['o']);
    loggerType = parseLogger(matches['l']);
    primsPerStCompEv = parseCompression(matches['c']);

    if (primsPerStCompEv == 1)
        genTCxt = ThreadContextGenerator<ThreadContextUncompressed>;
    else if (primsPerStCompEv > 1)
        genTCxt = ThreadContextGenerator<ThreadContextCompressed>;
    else
        fatal("SynchroTraceGen: Invalid compression level detected");
}

}; //end namespace STGen

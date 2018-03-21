#include "gn_threads.h"
#include "gn_events.h"
#include "gn_ipc.h"
#include "gn_debug.h"

#define GN_MAX_THREADS 500
// max threads 'running' at a given time
// same as MAX_THREADS_DEFAULT

static ThreadId threadIdCounter;
static ThreadId threadIdMap[GN_MAX_THREADS];
static ThreadState threadStateTable[GN_MAX_THREADS];
static ThreadId spawnerThread;
static Bool isInSyncCall;

ThreadId GN_(lastTid); // ThreadID of the last BB
ThreadId GN_(currentTid);

//-------------------------------------------------------------------------------------------------
/** Helper function definitions **/

static inline ThreadId getUTID(ThreadId tid)
{
    GN_ASSERT(threadIdMap[tid] != VG_INVALID_THREADID);
    return threadIdMap[tid];
}

static void switchThread(ThreadId tid)
{
    ThreadId utid = getUTID(tid);
    if (GN_(currentTid) == utid)
        return;

    /* save current state */
    threadStateTable[GN_(currentTid)].lastJmpsPassed = GN_(lastJmpsPassed);
    threadStateTable[GN_(currentTid)].isInSyncCall = isInSyncCall;
    threadStateTable[GN_(currentTid)].eventGenerationEnabled = GN_(EventGenerationEnabled);

    /* restore previous state */
    GN_(lastJmpsPassed) = threadStateTable[utid].lastJmpsPassed;
    GN_(currentTid) = utid;
    isInSyncCall = threadStateTable[utid].isInSyncCall;
    GN_(EventGenerationEnabled) = threadStateTable[GN_(currentTid)].eventGenerationEnabled;
}


//-------------------------------------------------------------------------------------------------
/** External function definitions **/

void GN_(preVGThreadCreate)(ThreadId parent, ThreadId child)
{
    if (GN_(clo).gen_sync == False)
        return;

    /* Create a new mapping from child TID -> reported TID
     *
     * This mapping is required because VG reuses exited thread ids,
     * and each new thread should not have its stats conflated with
     * previous threads with the same ID
     * 
     * VG thread ids are expected to count up, starting from 1 */
    GN_ASSERT(threadIdCounter <= GN_MAX_UNIQUE_THREADS);
    threadIdMap[child] = ++threadIdCounter; // expects first thread to be "1"

    /* The last thread to make a thread spawn call (pthread_create)
     * should be saved.
     * Now send the thread spawn event with the child unique id */
    GN_ASSERT(spawnerThread == parent);
    GN_ASSERT(GN_(lastTid) == parent);

    SyncID spawnData;
    spawnData = threadIdMap[child];

    GN_(flush_Sync)((UChar)PRISM_SYNC_CREATE, &spawnData, 1);
}


void GN_(preVGThreadExit)(ThreadId quitTid)
{
    if (GN_(clo).gen_sync == False)
        return;

    GN_ASSERT(threadIdCounter > quitTid);

    SyncID exitData;
    exitData = threadIdMap[quitTid];

    /* TODO(soonish)
     * MDL20180304 is sending the manner of exit (e.g. join) enough,
     * or do we walso send a PRISM_SYNC_EXIT? */
}


void GN_(initializeThreadState)(void)
{
    /* initialize Thread IDs */
    threadIdCounter = 0;

    for (UInt i=0; i<GN_MAX_THREADS; ++i)
        threadIdMap[i] = VG_INVALID_THREADID;

    spawnerThread = VG_INVALID_THREADID;
    GN_(lastTid) = VG_INVALID_THREADID;

    /* initialize per-thread state variables */
    for (UInt i=0; i<GN_MAX_THREADS; ++i) {
        threadStateTable[i].isInSyncCall = False;
        threadStateTable[i].eventGenerationEnabled = False;
    }
    isInSyncCall = False;
}


void GN_(preDeliverSignal)(ThreadId tid, Int sigNum,
                           __attribute__((unused)) Bool altStack)
{
    switchThread(tid);
    GN_DEBUG(1, "Pre Signal %d", sigNum);
}


void GN_(postDeliverSignal)(ThreadId tid, Int sigNum)
{
    switchThread(tid);
    GN_DEBUG(1, "Post Signal %d", sigNum);
}


void GN_(checkSwitchThread)(void)
{
    /* expected to be called at beginning of BB */

    ThreadId tid = VG_(get_running_tid)();
    if (GN_(lastTid) != tid) {

        GN_ASSERT(GN_(currEv) < GN_(endEv));

        /* add event */
        PrismEvVariant *slot = GN_(currEv);
        slot->tag = PRISM_SYNC_TAG;
        slot->sync.type = PRISM_SYNC_SWAP;
        slot->sync.data[0] = threadIdMap[tid];

        /* increment event slot */
        ++GN_(currEv);
        ++*GN_(usedEv);
        if (GN_(currEv) == GN_(endEv))
            GN_(flushCurrAndSetNextBuffer)();

        /* swap contexts */
        switchThread(tid);

        GN_(lastTid) = tid;
    }
}

void GN_(setInSyncCall)(ThreadId tid)
{
    if (GN_(currentTid) != tid)
        switchThread(tid);
    isInSyncCall = True;
    GN_(updateEventGeneration)();
}


void GN_(resetInSyncCall)(ThreadId tid)
{
    if (GN_(currentTid) != tid)
        switchThread(tid);
    isInSyncCall = False;
    GN_(updateEventGeneration)();
}


Bool GN_(isInSyncCall)(void)
{
    return isInSyncCall;
}

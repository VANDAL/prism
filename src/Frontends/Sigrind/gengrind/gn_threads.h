#ifndef GN_THREADS_H
#define GN_THREADS_H

#include "gn.h"
#include "pub_tool_threadstate.h"

#define GN_MAX_UNIQUE_THREADS 4096
// total threads created over span of target application

extern ThreadId GN_(lastTid);
extern ThreadId GN_(currentTid);

//-------------------------------------------------------------------------------------------------
/** Callstack-tracking type definitions **/

typedef struct _ThreadState ThreadState;
struct _ThreadState {
    UInt lastJmpsPassed;
    Bool isInSyncCall;
    Bool eventGenerationEnabled;
};


//-------------------------------------------------------------------------------------------------
/** Thread-tracking declarations **/

void GN_(preVGThreadCreate)(ThreadId parent, ThreadId child);
void GN_(preVGThreadExit)(ThreadId quitTid);
void GN_(initializeThreadState)(void);

void GN_(preDeliverSignal)(ThreadId tid, Int sigNum, Bool altStack);
void GN_(postDeliverSignal)(ThreadId tid, Int sigNum);

void GN_(checkSwitchThread)(void);

Bool GN_(isInSyncCall)(void);
void GN_(setInSyncCall)(ThreadId tid);
void GN_(resetInSyncCall)(ThreadId tid);

#endif

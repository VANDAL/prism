#ifndef GN_EVENTS_H
#define GN_EVENTS_H

#include "gn.h"
#include "gn_callstack.h"
#include "gn_jumps.h"

//-------------------------------------------------------------------------------------------------
/** Events infrastructure **/

typedef struct GN_(_EvVariant) GN_(EvVariant);
typedef struct GN_(_MemoryEvent) GN_(MemoryEvent);
typedef PrismCxtEv GN_(InstrEvent);
typedef PrismCompEv GN_(ComputeEvent);
typedef GnJumpKind GN_(JKEvent);
typedef struct GN_(Flush) GN_(Flush);


enum GN_(EvTag) {
    GN_UNDEF_EV,
    GN_MEMORY_EV,
    GN_COMPUTE_EV,
    GN_INSTR_EV,
    GN_JK_EV,
};


struct GN_(_MemoryEvent) {
    IRExpr *aexpr;
    IRExpr *guard;
    /* TODO the alt LoadG value is ignored for now */
    UInt size;
    Bool guarded;
    /* guarded == False implies guard == NULL, and vice-versa */
    Bool load;
};


struct GN_(_EvVariant) {
    enum GN_(EvTag) tag;
    union {
        GN_(MemoryEvent) mem;
        GN_(ComputeEvent) comp;
        GN_(InstrEvent) instr;
        GN_(JKEvent) jk;
    };
};


#define GN_MAX_EVENTS_PER_BB (1024)
extern GN_(EvVariant) GN_(EvBuffer)[GN_MAX_EVENTS_PER_BB];
/* A smaller buffer local to Valgrind
 * Used to track per-bb events */

#define GN_MAX_JUMPS_PER_BB (64)
extern UInt GN_(EvJumpsInBB);
extern GnJumpKind GN_(EvJumps)[GN_MAX_JUMPS_PER_BB];

extern Bool GN_(EventGenerationEnabled);


enum GN_(FlushTag) {
    GN_FLUSH_EXIT_ST,
    GN_FLUSH_BB_END,
};

struct GN_(Flush) {
    enum GN_(FlushTag) tag;

    Int exit_imark_idx;
    Int exit_stmt_idx;
    /* only valid if flush is on an Ist_Exit */
};


//-------------------------------------------------------------------------------------------------
void GN_(add_TrackSyncs)(BBState *bbState);
void GN_(flush_Sync)(SyncType type, SyncID *data, UInt args);

void GN_(add_TrackFns)(BBState *bbState);
void GN_(flush_FnExit)(const HChar *fnname);
void GN_(flush_FnEnter)(const HChar *fnname);

void GN_(addEvent_Instr)(BBState *bbState, const IRStmt *st);
void GN_(addEvent_Compute)(BBState *bbState, const IRStmt *st);
void GN_(addEvent_Memory_Load)(BBState *bbState, const IRStmt *st);
void GN_(addEvent_Memory_Store)(BBState *bbState, const IRStmt *st);
void GN_(addEvent_CAS)(BBState *bbState, const IRStmt *st);
void GN_(addEvent_LLSC)(BBState *bbState, const IRStmt *st);
void GN_(addEvent_Dirty)(BBState *bbState, const IRStmt *st);
void GN_(addEvent_Exit)(BBState *bbState, const IRStmt *st);
void GN_(addEvent_BBEnd)(BBState *bbState);

void GN_(flushEvents)(BBState *bbState, Int flush_from, GN_(Flush) flushType);

void GN_(updateEventGeneration)(void);

void GN_(addEvent_Memory_Guarded_Load)(BBState *bbState, const IRStmt *st);
void GN_(addEvent_Memory_Guarded_Store)(BBState *bbState, const IRStmt *st);
/* unsupported events */

#endif

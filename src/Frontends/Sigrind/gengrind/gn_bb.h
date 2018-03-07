#ifndef GN_BB_H
#define GN_BB_H

#include "gn.h"

extern BBTable GN_(allBBs);
extern BBInfo *GN_(lastBB);
extern ULong GN_(bbExecutions);
extern ULong GN_(bbSeen);

//-------------------------------------------------------------------------------------------------
/** BB-tracking type definitions **/

struct _BBState {
    /* Tracks the state during instrumentation.
     * Useful for passing between instrumentation functions */

    IRType hWordTy;

    IRSB *obb;
    // The original basic block

    IRSB *nbb;
    // The new basic block with instrumentation

    BBInfo *bbInfo;
    // metadata for this basic block
    
    UInt eventsToFlush;
    // Number of static events to-be-added for the current basic block
    // Events are held in a separate global array

    Int jmpsPassed;
};


struct _BBInfo {
    /* Useful metadata valid outside of instrumentation,
     * e.g. which function/ELF object contain this BB
     * Some vars are unused if function-tracking is not enabled */

    BBInfo* next;
    // chaining for hash collisions

    UInt eventsTotal;
    // events are periodically flushed before an exit

    PtrdiffT offset;
    // offset of BB in ELF object

    ObjNode *obj;
    VgSectKind sectKind;
    // ELF object

    FnNode *fn;
    Bool isFnEntry;
    // Do not access directly, use helper function getFnNode
    // Containing function; evaluated on-demand and cached
    // Only valid if function enter/exit events are enabled

    UInt jmpCount;
    GnJumpKind *jmps;
    JumpNode *lruToJmp;
    JumpNode *lruFromJmp;
    Bool lastJmpInverted;
    // all side exits
    // Only valid if certain events are enabled (e.g. functions)
    
    UInt uid;
    UInt instrCount;
    // for debugging purposes
};


struct _BBTable {
    /* all basic blocks saved here */
    UInt entries;
    UInt capacity;
    BBInfo **table;
};

//-------------------------------------------------------------------------------------------------
/** BB-tracking function declarations **/

void GN_(initBB)(void);
void GN_(initAllBBsTable)(void);

BBInfo* GN_(getBBInfo)(Addr addr);
Int GN_(initBBState)(BBState *bbState, IRSB *origBB, IRType hWordTy);

#endif

#ifndef GN_CALLSTACK_H
#define GN_CALLSTACK_H

/* Manually track callstacks for function entry/exit event generation
 *
 * Callgrind's callstack tracking implementation is used as a base for
 * this implementation
 *
 * In general, tracking function contexts is demanding for a number
 * of reasons including:
 * - availability of debug info
 * - varying calling conventions 
 * - frequency of calls
 * - shared library mapping
 * - object loading
 * - IR manipulations
 */

#include "gn.h"

extern UInt GN_(lastJmpsPassed);
extern Bool GN_(afterStartFunc);
extern Bool GN_(afterEndFunc);

//-------------------------------------------------------------------------------------------------
/** Callstack-tracking type definitions **/

struct _CallEntry {
    /* An entry in the callstack */

    JumpNode* jn;
    /* JumpNode for this call */

    Addr sp;
    /* stack pointer directly after this call (at the beginning of the call) */

    Addr ret_addr;
    /* ML: not implemented, see Callgrind if needed
     *
     * Address to which to return to
     * Is 0 on a simulated call
     *
     * Return addr only useful for REAL CALL
     * Used to detect RETURN w/o CALL */

    Int fn_sp;
    /* unused, function stack index before call */
};


struct _CallStack {
    /* The full callstack */

    Int tos;
    // top-of-stack/idx into call entries

    UInt capacity;
    CallEntry* entry;
};


struct _ThreadInfo {
    /* Thread State
     *
     * This structure stores thread specific info while a thread is *not* running.
     */

    /* state */
    CallStack calls;   /* context call arc stack */
//  ExecStack states;  /* execution states interrupted by signals */
//
//  /* thread specific data structure containers */
//  fn_array fn_active;
//  jcc_hash jccs;
//  bbcc_hash bbccs;
};


//-------------------------------------------------------------------------------------------------
/** Callstack-tracking function declarations **/

void GN_(trackFns)(BBInfo *thisBB);
void GN_(initCallStack)(void);
void finishCallstack(void);

#endif

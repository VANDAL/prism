#include "gn.h"
#include "gn_events.h"
#include "gn_bb.h"
#include "gn_fn.h"
#include "gn_callstack.h"
#include "gn_jumps.h"
#include "gn_debug.h"
#include "pub_tool_threadstate.h"


static CallStack currentCallStack;
Bool GN_(afterStartFunc);
Bool GN_(afterEndFunc);


static inline Bool isFirstBB(void) { return GN_(lastBB) == NULL; }


#define N_CALL_STACK_INITIAL_ENTRIES 500
static void initCallStack(CallStack *s)
{
    GN_ASSERT(s != NULL);
    s->capacity = N_CALL_STACK_INITIAL_ENTRIES;
    s->entry = VG_(malloc)("gn.callstack.initcallstack.1",
                           s->capacity * sizeof(CallEntry));
    s->tos = 0;
}


static void resizeCurrentCallStack(void)
{
    if ((Long)currentCallStack.tos < currentCallStack.capacity)
        return;

    UInt newcap = 2 * currentCallStack.capacity;
    currentCallStack.entry = VG_(realloc)("gn.callstack.resize.1",
                                          currentCallStack.entry,
                                          newcap * sizeof(CallEntry));
    currentCallStack.capacity = newcap;
}


typedef struct _GnCallstackJumpKind {
    /* package extra data depending on jump kind */
    GnJumpKind jk;
    Bool wasConditionalJmp;
    union {
        Int popCountOnReturn;
        /* if jk_Return, what is the minimum number of pops to the callstack? */

        Bool callEmulation;
        /* if jk_Call, is this actual call or faked */
    };
} GnCallstackJumpKind;


static Addr adjustSP(GnCallstackJumpKind cjk, Addr sp)
{
    Int cstop = currentCallStack.tos;
    Addr newsp = sp;

    if (cjk.jk == jk_Return) {
        GN_ASSERT(cjk.popCountOnReturn == 1);
    }
    else {
        if (cjk.jk == jk_Call &&
            cjk.callEmulation == True &&
            cstop > 0) {

            /* If there wasn't really a CALL,
             * then use the SP from last actual CALL
             * This is useful for stack unwinding */
            newsp = (currentCallStack.entry[cstop-1]).sp;
        }
    }

    return newsp;
}


static GnCallstackJumpKind determineJumpKind(BBInfo *bb, Addr sp)
{
    GN_DEBUG(6, "+ determineJumpKind\n");

    GnJumpKind jmpkind;
    Bool wasConditionalJmp;
    Bool callEmulation = False;

    GN_DEBUGIF(4) {
        GN_(printBBno)();
        GN_(printTabs)(1);
        VG_(printf)("SP: 0x%lx\n", sp);
    }

    if (isFirstBB()) {
        jmpkind = jk_None;
        wasConditionalJmp = False;
    }
    else {
        GN_DEBUGIF(4) {
            if (GN_(lastBB)->jmpCount < GN_(lastJmpsPassed)) {
                GN_(printBBno)();
                VG_(printf)("%d %d\n", GN_(lastBB)->jmpCount, GN_(lastJmpsPassed));
            }
        }

        GN_ASSERT(GN_(lastBB)->jmpCount >= GN_(lastJmpsPassed));

        jmpkind = GN_(lastBB)->jmps[GN_(lastJmpsPassed)];
        wasConditionalJmp = GN_(lastJmpsPassed) < (GN_(lastBB)->jmpCount-1);

        GN_DEBUGIF(4) {
            GN_(printBBno)();
            BBInfo *lastbb = GN_(lastBB);
            if (lastbb) {
                HChar *from = lastbb->fn->name;
                HChar *to = bb->fn->name;
                UInt fromId = lastbb->uid;
                UInt toId = bb->uid;
                GN_(printTabs)(1);
                VG_(printf)("JmpKind: %s; From BB/Fn: %d/%s to To BB/Fn: %d/%s\n",
                            GN_(getJumpStr)(jmpkind, wasConditionalJmp),
                            fromId, from,
                            toId, to);
            }
        }
    }

    Int popCountOnReturn = 1;
    Bool retWithoutCall = False;

    Int cstop = currentCallStack.tos;
    Int up_one = cstop-1;
    CallEntry *topce = &(currentCallStack.entry[up_one]); // The call entry just returned from

    if ((jmpkind == jk_Return) && (cstop > 0)) {
        /* Check if there is a return that doesn't match up with the callstack,
         * in which case it will be treated as a normal jump */

        if (sp < topce->sp) {
            /* If current sp < last call sp (sp still in frame),
             * then this is not a 'real' return.
             * MDL20180103 When does this happen? */
            popCountOnReturn = 0;

            GN_DEBUGIF(4) {
                VG_(printf)("\tReturn detected but still in frame:"
                            " Current SP - 0x%lx, Top SP - 0x%lx\n", sp, topce->sp);
            }
        }
        else if (sp == topce->sp) {
            /* This is a special case that Callgrind handles for PPC
             * Gengrind doesn't support this yet
             * Callgrind modifies popCountOnReturn here */
            VG_(printf)("Error: unexpected stack pointer\n"
                        "Is this a supported platform?");
            GN_ASSERT(False);
        }
        else {
            // Typical case by convention; no special handling
        }

        if (popCountOnReturn == 0) {
            jmpkind = jk_Jump;
            retWithoutCall = True;
        }
    }

    if ((jmpkind != jk_Return) && (jmpkind != jk_Call) && !isFirstBB()) {
        if (retWithoutCall ||
            bb->isFnEntry ||
            (GN_(lastBB)->sectKind != bb->sectKind) ||
            (GN_(lastBB)->obj->number != bb->obj->number)) {
            /* a JMP/Cont is converted to a CALL if:
             * - the jump is to another ELF object or section
             * - the jump is to the beginning of a function (tail recursion) */
            jmpkind = jk_Call;
            callEmulation = True;
        }
        else if (sp < topce->sp) {
            /* Sometimes the SP will be pushed within the same function,
             * and won't be popped to the same SP
             */
        }
    }

    GnCallstackJumpKind cjk;
    cjk.jk = jmpkind;
    cjk.wasConditionalJmp = wasConditionalJmp;
    if (cjk.jk == jk_Call) {
        cjk.callEmulation = callEmulation;
    }
    else {
        cjk.popCountOnReturn = popCountOnReturn;
    }

    GN_DEBUG(6, "- determineJumpKind\n");

    return cjk;
}


static void pushCallStack(BBInfo *from, UInt jmp,
                          BBInfo *to, Addr sp)
{
    /* assume first BB is not called */
    GN_ASSERT(from != NULL);

    FnNode *to_fn = to->fn;

    /* set entry in callstack */

    CallEntry *callee = &currentCallStack.entry[currentCallStack.tos];
    callee->sp = sp;

    /* signal this function is skipped by NULLing jn */
    JumpNode *jn;
    if (to_fn->skip == True)
        jn = NULL;
    else
        jn = GN_(getJumpNode)(from, jmp, to);
    callee->jn = jn;

    if (GN_(clo).start_collect_func != NULL &&
            GN_(afterStartFunc) == False &&
            VG_(strcmp)(to_fn->name, GN_(clo).start_collect_func) == 0) {
        GN_(afterStartFunc) = True;
        GN_(updateEventGeneration)();
    }

    if (to_fn->skip == False) {
        GN_(flush_FnEnter)(to_fn->name);
    }

    currentCallStack.tos++;

    if ((Long)currentCallStack.tos >= currentCallStack.capacity)
        resizeCurrentCallStack();

    GN_DEBUGIF(4) {
        VG_(printf)("\tPUSH: JN - %p, SP - 0x%lx\n", jn, sp);
    }
}


static void popCallStack(void)
{
    CallEntry *caller = &currentCallStack.entry[currentCallStack.tos-1];
    JumpNode *jn = caller->jn;

    if (jn != NULL) {
        FnNode *to_fn = jn->to->fn;

        if (GN_(clo).stop_collect_func != NULL &&
                GN_(afterEndFunc) == False &&
                VG_(strcmp)(to_fn->name, GN_(clo).stop_collect_func) == 0) {
            GN_(afterEndFunc) = True;
            GN_(updateEventGeneration)();
        }

        GN_(flush_FnExit)(to_fn->name);
    }

    currentCallStack.tos--;

    GN_DEBUGIF(4) {
        VG_(printf)("\tPOP: %p\n", jn);
    }
}


static Int unwindCurrentCallStack(Addr sp, Int minpops)
{
    GN_DEBUG(6, "+ unwindCurrentCallStack (sp = 0x%lx, minpops = %d)\n", sp, minpops);
    /* Assume general (x86?) calling conventions
     * After CALL: SP = p
     * After RETURN: SP = s
     * Generally, s>p, because the RETURN addr is always additionally popped by
     * the callee */

    Int unwinds = 0;
    Int cstop = currentCallStack.tos;
    while (cstop > 0) {
        CallEntry *ce = &(currentCallStack.entry[cstop-1]);
        if (ce->sp < sp ||
            (ce->sp == sp && minpops>0)) {
            /* ML: when does ce->sp == sp ? Some cases exist in amd64 */

            GN_DEBUGIF(4) {
                GN_(printTabs)(1);
                VG_(printf)("Popping: CurrentSP - 0x%lx; TopSP - 0x%lx\n",
                            sp, ce->sp);
            }

            popCallStack();
            cstop = currentCallStack.tos;
            unwinds++;
            minpops--;
            continue;
        }
        break;
    }

    GN_DEBUG(6, "- unwindCurrentCallStack\n");

    return unwinds;
}

//-------------------------------------------------------------------------------------------------
/** External function definitions **/

void GN_(trackFns)(BBInfo *bb)
{
    GN_DEBUG(6, "+ trackFns\n");

    /* Check how this BB was entered (call/return/jmp/et al from prevous BB)
     * and manually track the callstack.
     * This is a more general and robust method than tracking CALL/RETURN
     * instrs, at the cost of additional overhead */

    Addr sp = VG_(get_SP)(VG_(get_running_tid)());
    GnCallstackJumpKind cjk = determineJumpKind(bb, sp);
    sp = adjustSP(cjk, sp);

    if (cjk.jk == jk_Call) {
        pushCallStack(GN_(lastBB), GN_(lastJmpsPassed), bb, sp);
    }
    else if (cjk.jk == jk_Return) {
        unwindCurrentCallStack(sp, cjk.popCountOnReturn);
    }
    else {
        /* Callgrind does an unwind, anyway, to check if there was a return.
         * jk_Return seems to be only used if the last exit of a BB is a return,
         * and is never seen on conditional jumps earlier in the BB. */
        Int unwinds = unwindCurrentCallStack(sp, 0);
        if (unwinds > 0)
            cjk.jk = jk_Return;
    }

    GN_DEBUGIF(4) {
        if (GN_(lastBB)) {
            Int jmpkind = GN_(lastBB)->jmps[GN_(lastJmpsPassed)];
            if (jmpkind != cjk.jk) {
                GN_(printTabs)(1);
                VG_(printf)("JumpKind adjusted to: %s\n", GN_(getJumpStr)(cjk.jk,
                                                                          cjk.wasConditionalJmp));
            }
            Addr origSP = VG_(get_SP)(VG_(get_running_tid)());
            if (origSP != sp) {
                GN_(printTabs)(1);
                VG_(printf)("SP adjusted to: 0x%lx\n", sp);
            }
        }
    }

    /* Setup state for next BB */
    /* TODO(someday) Clean up and add this in another dirty call at the end of the BB */
    GN_(lastBB) = bb;
    GN_(lastJmpsPassed) = 0;

    GN_(bbExecutions)++;
}


void GN_(initCallStack)()
{
    initCallStack(&currentCallStack);
    GN_(afterStartFunc) = True;
    GN_(afterEndFunc) = False;
}


void finishCallstack()
{
    while (currentCallStack.tos > 0)
        popCallStack();
}

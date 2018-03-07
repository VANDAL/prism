#include "gn.h"
#include "gn_events.h"
#include "gn_ipc.h"
#include "gn_clo.h"
#include "gn_callstack.h"
#include "gn_threads.h"
#include "gn_bb.h"
#include "gn_debug.h"

#define UNUSED_SYNC_DATA 0
#define MAX_SYNC_DATA 2

//-------------------------------------------------------------------------------------------------
/* Events are added to a temporary buffer at instrumentation-time,
 * in the order they are seen in the basic block.
 * When an 'exit' IR stmt is found, all added events up to that point
 * are flushed(instrumented) into the basic block, before the 'exit'.
 */

Bool GN_(EventGenerationEnabled);

//-------------------------------------------------------------------------------------------------
/** Global BB event tracking definitions **/
GN_(EvVariant) GN_(EvBuffer)[GN_MAX_EVENTS_PER_BB];
GnJumpKind GN_(EvJumps)[GN_MAX_JUMPS_PER_BB];


//-------------------------------------------------------------------------------------------------
/** Events added at instrumentation-time **/

void GN_(add_TrackSyncs)(BBState *bbState)
{
    /* This checks if the current running thread was just swapped.
     * Other sync events, like thread spawn/join/lock etc, are detected
     * via thread library API wrapping, via Valgrind's client request mechanism */

    IRExpr **argv = mkIRExprVec_0();
    IRDirty *di = unsafeIRDirty_0_N(0, "checkSwitchThread",
                                    VG_(fnptr_to_fnentry)(GN_(checkSwitchThread)), argv);
    addStmtToIRSB(bbState->nbb, IRStmt_Dirty(di));
}


void GN_(add_TrackFns)(BBState *bbState)
{
    /* Track any change in the state of the running program that is checked
     * at the beginning of a BB, e.g. a thread context change, function entry,
     * et al
     *
     * This is expected to be instrumented at the beginning of the BB */

    IRExpr **argv = mkIRExprVec_1(mkIRExpr_HWord((HWord)bbState->bbInfo));
    IRDirty *di = unsafeIRDirty_0_N(1, "trackFns",
                                    VG_(fnptr_to_fnentry)(GN_(trackFns)), argv);
    addStmtToIRSB(bbState->nbb, IRStmt_Dirty(di));
}


void GN_(addEvent_Instr)(BBState *bbState, const IRStmt *st)
{
    GN_DEBUG(6, "+ addEvent_Instr\n");

    GN_ASSERT(st->tag == Ist_IMark);
    if (GN_(clo).gen_instr == False)
        return;

    Addr   cia   = st->Ist.IMark.addr + st->Ist.IMark.delta;
    UInt   isize = st->Ist.IMark.len;

    // If Vex fails to decode an instruction, the size will be zero.
    // Pretend otherwise.
    if (isize == 0) isize = VG_MIN_INSTR_SZB;

    // Sanity-check size.
    tl_assert( (VG_MIN_INSTR_SZB <= isize && isize <= VG_MAX_INSTR_SZB)
               || VG_CLREQ_SZB == isize );

    GN_ASSERT(bbState->eventsToFlush < GN_MAX_EVENTS_PER_BB);
    GN_(EvVariant) *ev = GN_(EvBuffer) + bbState->eventsToFlush;
    ev->tag = GN_INSTR_EV;
    ev->instr.type = SGLPRIM_CXT_INSTR;
    ev->instr.id = cia;

    bbState->eventsToFlush++;

    GN_DEBUGIF(4) {
        GN_(printTabs)(1);
        VG_(printf)("Added Event: Instr - ");
        VG_(printf)("Addr: %lu", cia);
        VG_(printf)("\n");
    }

    GN_DEBUG(6, "- addEvent_Instr\n");
}


void GN_(addEvent_Compute)(BBState *bbState, const IRStmt *st)
{
    GN_DEBUG(6, "+ addEvent_Compute\n");

    GN_ASSERT(st->tag == Ist_WrTmp);
    if (GN_(clo).gen_comp == False)
        return;

    GN_ASSERT(bbState->eventsToFlush < GN_MAX_EVENTS_PER_BB);
    GN_(EvVariant) *ev = GN_(EvBuffer) + bbState->eventsToFlush;

    ev->tag = GN_COMPUTE_EV;
    ev->comp.type = SGLPRIM_COMP_TYPE_UNDEF;
    ev->comp.arity = SGLPRIM_COMP_ARITY_UNDEF;
    ev->comp.op = SGLPRIM_COMP_OP_UNDEF;
    ev->comp.size = 0;

    IRExpr* data = st->Ist.WrTmp.data;
    IRType type = typeOfIRExpr(bbState->nbb->tyenv, data);
    IRExprTag arity = data->tag;

    if (type > Ity_INVALID) {
        if (type < Ity_F16) {
            ev->comp.type = SGLPRIM_COMP_IOP;
        }
        else if (type < Ity_V128) {
            ev->comp.type = SGLPRIM_COMP_FLOP;
        }
        else {
            //VG_(umsg)("Unsupported SIMD event encountered\n");
            //ppIRStmt(bbState->st); VG_(printf)("\n");
            /* SIMD */
        }
    }

    switch (arity) {
    case Iex_Unop:
        ev->comp.arity = SGLPRIM_COMP_UNARY;
        break;
    case Iex_Binop:
        ev->comp.arity = SGLPRIM_COMP_BINARY;
        break;
    case Iex_Triop:
        ev->comp.arity = SGLPRIM_COMP_TERNARY;
        break;
    case Iex_Qop:
        ev->comp.arity = SGLPRIM_COMP_QUARTERNARY;
        break;
    default:
        tl_assert(0);
        break;
    }

    /* TODO op mappings  */
    /* TODO size mappings */

    bbState->eventsToFlush++;

    GN_DEBUGIF(4) {
        GN_(printTabs)(1);
        VG_(printf)("Added Event: Compute - ");
        VG_(printf)("type: %d", ev->comp.type);
        VG_(printf)("\n");
    }

    GN_DEBUG(6, "- addEvent_Compute\n");
}


static void gnSetEvent_Memory(GN_(EvVariant) *ev, Bool load, IRExpr *guard,
                              IRExpr *aexpr, UInt size)
{
    ev->tag = GN_MEMORY_EV;
    ev->mem.load = load;
    ev->mem.guard = guard;
    ev->mem.guarded = guard == NULL ? False : True;
    ev->mem.aexpr = aexpr;
    ev->mem.size = size;

    GN_DEBUGIF(4) {
        GN_(printTabs)(1);
        VG_(printf)("Added Event: Memory - ");
        ev->mem.load ? VG_(printf)("load, ") : VG_(printf)("store, ");
        VG_(printf)("addr: %p", aexpr);
        VG_(printf)("\n");
    }
}


void GN_(addEvent_Memory_Load)(BBState *bbState, const IRStmt *st)
{
    GN_DEBUG(6, "+ addEvent_Memory_Load\n");

    GN_ASSERT(st->tag == Ist_WrTmp);
    if (GN_(clo).gen_mem == False)
        return;

    GN_ASSERT(bbState->eventsToFlush < GN_MAX_EVENTS_PER_BB);
    GN_(EvVariant) *ev = GN_(EvBuffer) + bbState->eventsToFlush;

    IRExpr* data = st->Ist.WrTmp.data;
    gnSetEvent_Memory(ev, True, NULL, data->Iex.Load.addr,
                      sizeofIRType(data->Iex.Load.ty));
    bbState->eventsToFlush++;

    GN_DEBUG(6, "- addEvent_Memory_Load\n");
}


void GN_(addEvent_Memory_Store)(BBState *bbState, const IRStmt *st)
{
    GN_DEBUG(6, "+ addEvent_Memory_Store\n");

    if (GN_(clo).gen_mem == False)
        return;

    GN_ASSERT(bbState->eventsToFlush < GN_MAX_EVENTS_PER_BB);
    GN_(EvVariant) *ev = GN_(EvBuffer) + bbState->eventsToFlush;

    IRExpr* data = st->Ist.Store.data;
    gnSetEvent_Memory(ev, False, NULL, st->Ist.Store.addr,
                      sizeofIRType(typeOfIRExpr(bbState->nbb->tyenv, data)));
    bbState->eventsToFlush++;

    GN_DEBUG(6, "- addEvent_Memory_Store\n");
}


void GN_(addEvent_Memory_Guarded_Load)(BBState *bbState, const IRStmt *st)
{
    GN_DEBUG(6, "+ addEvent_Memory_Guarded_Load\n");

    GN_ASSERT(st->tag == Ist_LoadG);
    if (GN_(clo).gen_mem == False)
        return;

    IRLoadG *lg = st->Ist.LoadG.details;
    IRType res, arg;
    typeOfIRLoadGOp(lg->cvt, &res, &arg);

    GN_ASSERT(bbState->eventsToFlush < GN_MAX_EVENTS_PER_BB);
    GN_(EvVariant) *ev = GN_(EvBuffer) + bbState->eventsToFlush;

    // We'll only capture the addr if the load happens,
    // otherwise ignore the memory event
    gnSetEvent_Memory(ev, True, lg->guard, lg->addr,
                      sizeofIRType(arg));
    bbState->eventsToFlush++;

    GN_DEBUG(6, "- addEvent_Memory_Guarded_Load\n");
}


void GN_(addEvent_Memory_Guarded_Store)(BBState *bbState, const IRStmt *st)
{
    GN_DEBUG(6, "+ addEvent_Memory_Guarded_Store\n");

    GN_ASSERT(st->tag == Ist_StoreG);
    if (GN_(clo).gen_mem == False)
        return;

    IRStoreG *sg = st->Ist.StoreG.details;

    GN_ASSERT(bbState->eventsToFlush < GN_MAX_EVENTS_PER_BB);
    GN_(EvVariant) *ev = GN_(EvBuffer) + bbState->eventsToFlush;

    gnSetEvent_Memory(ev, False, sg->guard, sg->addr,
                      sizeofIRType(typeOfIRExpr(bbState->nbb->tyenv, sg->data)));
    bbState->eventsToFlush++;

    GN_DEBUG(6, "- addEvent_Memory_Guarded_Store\n");
}


void GN_(addEvent_Dirty)(BBState *bbState, const IRStmt *st)
{
    /* Only memory events are generated for a Dirty IR,
     * since that's all the info VEX provides.
     *
     * Dirty IR stmts are rare and for special cases,
     * so any compute events would most likely be insignificant */

    GN_DEBUG(6, "+ addEvent_Dirty\n");

    GN_ASSERT(st->tag == Ist_Dirty);
    if (GN_(clo).gen_mem == False)
        return;

    IRDirty *di = st->Ist.Dirty.details;

    if (di->guard != NULL) {
        GN_ASSERT(isIRAtom(di->guard));

        IRConst *t = IRConst_U1(True);
        Bool isGuarded = (di->guard->tag != Iex_Const ||
                          !eqIRConst(di->guard->Iex.Const.con, t));

        if (isGuarded) {
            /* if it's guarded by anything but 'Const(True)', then return early */
            VG_(umsg)("Unsupported Guarded Dirty Call encountered\n");
            ppIRStmt(st); VG_(printf)("\n");
            ppIRExpr(di->guard); VG_(printf)("\n");
            isGuarded = True;
            return;
        }
    }

    if (di->mFx != Ifx_None) {
        tl_assert(di->mAddr != NULL);
        tl_assert(di->mSize != 0);
        UInt datasize = di->mSize;
        if (di->mFx == Ifx_Read || di->mFx == Ifx_Modify) {
            GN_ASSERT(bbState->eventsToFlush < GN_MAX_EVENTS_PER_BB);
            GN_(EvVariant) *ev = GN_(EvBuffer) + bbState->eventsToFlush;
            gnSetEvent_Memory(ev, True, NULL, di->mAddr, datasize);
            bbState->eventsToFlush++;
        }
        if (di->mFx == Ifx_Write || di->mFx == Ifx_Modify)
            GN_ASSERT(bbState->eventsToFlush < GN_MAX_EVENTS_PER_BB);
            GN_(EvVariant) *ev = GN_(EvBuffer) + bbState->eventsToFlush;
            gnSetEvent_Memory(ev, False, NULL, di->mAddr, datasize);
            bbState->eventsToFlush++;
    }
    else {
        tl_assert(di->mAddr == NULL);
        tl_assert(di->mSize == 0);
    }

    GN_DEBUGIF(4) {
        GN_(printTabs)(1);
        VG_(printf)("Added Event: Dirty Call\n");
    }

    GN_DEBUG(6, "- addEvent_Dirty\n");
}


void GN_(addEvent_CAS)(BBState *bbState, const IRStmt *st)
{
    /* Follow Callgrind's analysis of this event
     * as a read, followed by a write */

    // TODO send a special event?

    GN_DEBUG(6, "+ addEvent_CAS\n");

    GN_ASSERT(st->tag == Ist_CAS);
    if (GN_(clo).gen_mem == False)
        return;

    IRCAS *cas = st->Ist.CAS.details;
    UInt datasize = sizeofIRType(typeOfIRExpr(bbState->nbb->tyenv, cas->dataLo));
    if (cas->dataHi != NULL)
        datasize *= 2; /* since this is a doubleword-cas */

    GN_ASSERT(bbState->eventsToFlush < GN_MAX_EVENTS_PER_BB);
    GN_(EvVariant) *ev = GN_(EvBuffer) + bbState->eventsToFlush;

    gnSetEvent_Memory(ev, True, NULL, cas->addr, datasize);
    bbState->eventsToFlush++;

    ev = GN_(EvBuffer) + bbState->eventsToFlush;
    gnSetEvent_Memory(ev, False, NULL, cas->addr, datasize);

    bbState->eventsToFlush++;

    GN_DEBUGIF(4) {
        GN_(printTabs)(1);
        VG_(printf)("Added Event: CAS\n");
    }

    GN_DEBUG(6, "- addEvent_CAS\n");
}


void GN_(addEvent_LLSC)(BBState *bbState, const IRStmt *st)
{
    // TODO(someday) send a special event?
    //
    GN_DEBUG(6, "+ addEvent_LLSC\n");

    GN_ASSERT(st->tag == Ist_LLSC);
    if (GN_(clo).gen_mem == False)
        return;

    if (st->Ist.LLSC.storedata == NULL) {
        /* LL */
        UInt datasize =  sizeofIRType(typeOfIRTemp(bbState->nbb->tyenv, st->Ist.LLSC.result));
        GN_(EvVariant) *ev = GN_(EvBuffer) + bbState->eventsToFlush;
        gnSetEvent_Memory(ev, True, NULL, st->Ist.LLSC.addr, datasize);
        bbState->eventsToFlush++;
    }
    else {
        /* SC */
        UInt datasize =  sizeofIRType(typeOfIRExpr(bbState->nbb->tyenv, st->Ist.LLSC.storedata));
        GN_(EvVariant) *ev = GN_(EvBuffer) + bbState->eventsToFlush;
        gnSetEvent_Memory(ev, False, NULL, st->Ist.LLSC.addr, datasize);
        bbState->eventsToFlush++;
    }

    GN_DEBUGIF(4) {
        GN_(printTabs)(1);
        VG_(printf)("Added Event: LL/SC\n");
    }

    GN_DEBUG(6, "- addEvent_LLSC\n");
}


static void gnAddExit(BBState *bbState, IRJumpKind jk)
{
    GnJumpKind jmp;
    switch (jk) {
        /* N.B. empirically it looks like Ijk_Call/Ret are rarely,
         * if ever found, at least in amd64 */
    case Ijk_Boring:
        jmp = jk_Jump;
        break;
    case Ijk_Call:
        jmp = jk_Call;
        break;
    case Ijk_Ret:
        jmp = jk_Return;
        break;
    default:
        /* other types may be e.g. client requests, signals, et al
         * and are not recorded in the BB metadata */
        jmp = jk_Other;
        break;
    }

    /* Hold a running list of jumps for this BB (during instrumentation-time)
     * in a temporary global buffer before assigning to the BB metadata at the
     * end of the BB instrumentation */
    GN_(EvJumps)[bbState->bbInfo->jmpCount++] = jmp;

    GN_DEBUGIF(4) {
        GN_(printTabs)(1);
        VG_(printf)("Added Event: Exit - %s:%d\n",
                    GN_(getJumpStr)(jmp, False), bbState->bbInfo->jmpCount);
    }
}


void GN_(addEvent_Exit)(BBState *bbState, const IRStmt *st)
{
    GN_DEBUG(6, "+ addEvent_Exit\n");

    GN_ASSERT(st->tag == Ist_Exit);

    if (GN_(clo).gen_fn == False)
        return;

    gnAddExit(bbState, st->Ist.Exit.jk);

    GN_DEBUG(6, "- addEvent_Exit\n");
}


static void addEvent_BBEnd_jmps(BBState *bbState)
{
    GN_ASSERT(GN_(clo).gen_fn == True);

    gnAddExit(bbState, bbState->nbb->jumpkind);

    /* all exits */
    GN_ASSERT(bbState->bbInfo != NULL && bbState->bbInfo->jmps == NULL);
    bbState->bbInfo->jmps = VG_(malloc)("gn.events.jmps.1",
                                        bbState->bbInfo->jmpCount * sizeof(GnJumpKind));

    /* update the BBInfo now that we know about all exits */
    for (UInt i=0; i<bbState->bbInfo->jmpCount; ++i)
        bbState->bbInfo->jmps[i] = GN_(EvJumps)[i];
}


void GN_(addEvent_BBEnd)(BBState *bbState)
{
    /* Update BB metadata with any outstanding state gathered
     * during instrumentation phase */

    if (GN_(clo).gen_fn == True)
        addEvent_BBEnd_jmps(bbState);
}


//-------------------------------------------------------------------------------------------------
/** Instrumentation for events **/

static void gnReserveEventsInBuffer(IRSB *nbb, IRType tyW, UInt eventsToFlush)
{
    /* check if there's enough space in the event buffer */

    /* tmp1 <- GN_(currEv) */
    IRTemp currBufPtrTmp = newIRTemp(nbb->tyenv, tyW);
    IRExpr *currBufPtr = mkIRExpr_HWord((HWord)&GN_(currEv));
    addStmtToIRSB(nbb,
                  IRStmt_WrTmp(currBufPtrTmp,
                               IRExpr_Load(ENDNESS, tyW,
                                           currBufPtr)));

    /* tmp2 <- GN_(endEv) */
    IRTemp endBufPtrTmp = newIRTemp(nbb->tyenv, tyW);
    IRExpr *endBufPtr = mkIRExpr_HWord((HWord)&GN_(endEv));
    addStmtToIRSB(nbb,
                  IRStmt_WrTmp(endBufPtrTmp,
                               IRExpr_Load(ENDNESS, tyW,
                                           endBufPtr)));

    /* tmp3 <- size of events */
    UInt eventsBytes = eventsToFlush * sizeof(SglEvVariant);
    IRTemp eventsTmp = newIRTemp(nbb->tyenv, tyW);
    addStmtToIRSB(nbb,
                  IRStmt_WrTmp(eventsTmp,
                               IRExpr_Const(IRConst_U64(eventsBytes))));

    /* tmp4 = tmp1 + tmp3 */
    IRTemp neededTmp = newIRTemp(nbb->tyenv, tyW);
    IRExpr *calcNeeded = IRExpr_Binop(IOP_ADD_PTR,
                                      IRExpr_RdTmp(currBufPtrTmp),
                                      IRExpr_RdTmp(eventsTmp));
    addStmtToIRSB(nbb, IRStmt_WrTmp(neededTmp, calcNeeded));

    /* tmp5 = tmp2 < tmp4 */
    IRTemp cmpTmp = newIRTemp(nbb->tyenv, Ity_I1);
    IRExpr *cmp = IRExpr_Binop(IOP_CMPLT_PTR,
                               IRExpr_RdTmp(endBufPtrTmp),
                               IRExpr_RdTmp(neededTmp));
    addStmtToIRSB(nbb, IRStmt_WrTmp(cmpTmp, cmp));

    /* request new buffer if we ran out of events */
    IRDirty *di = emptyIRDirty();
    di->cee = mkIRCallee(0, "ipc.setNextBuffer",
                         VG_(fnptr_to_fnentry)(GN_(flushCurrAndSetNextBuffer)));
    di->guard = IRExpr_RdTmp(cmpTmp);
    di->args = mkIRExprVec_0();
    di->tmp = IRTemp_INVALID;
    di->nFxState = 0;
    di->mFx = Ifx_None;
    di->mAddr = NULL;
    di->mSize = 0;
    addStmtToIRSB(nbb, IRStmt_Dirty(di));
}


static IRExpr* gnIRConst(IRType type, UInt val)
{
    switch (type) {
    case Ity_I8:
        return IRExpr_Const(IRConst_U8(val));
        break;
    case Ity_I16:
        return IRExpr_Const(IRConst_U16(val));
        break;
    case Ity_I32:
        return IRExpr_Const(IRConst_U32(val));
        break;
    case Ity_I64:
        return IRExpr_Const(IRConst_U64(val));
        break;
    default:
        tl_assert(0);
    }
}


static IRTemp incrSlot(IRSB *bb, IRTemp oldSlotTmp, IRExpr *slotSize, IRType tyW)
{
    /* increment event slot
     * VEX IR is SSA, so we need to get a new temporary */
    IRTemp slotTmp = newIRTemp(bb->tyenv, tyW);
    addStmtToIRSB(bb,
                  IRStmt_WrTmp(slotTmp,
                               IRExpr_Binop(IOP_ADD_PTR,
                                            IRExpr_RdTmp(oldSlotTmp), slotSize)));
    return slotTmp;
}


#define GN_STORE_CONST_TO_OFFSET(bb, slot, val, type, member) {\
        IRTemp tmp = newIRTemp(bb->tyenv, IRTYPE_PTR);\
        IRExpr *offsetExpr = mkIRExpr_HWord((HWord)offsetof(type, member));\
        addStmtToIRSB(bb,\
                      IRStmt_WrTmp(tmp,\
                                   IRExpr_Binop(IOP_ADD_PTR,\
                                                IRExpr_RdTmp(slot), offsetExpr)));\
        IRType itype = integerIRTypeOfSize(MEMBER_SIZE(type, member));\
        addStmtToIRSB(bb,\
                      IRStmt_Store(ENDNESS,\
                                   IRExpr_RdTmp(tmp),\
                                   gnIRConst(itype, val)));\
    }

#define GN_STORE_EXPR_TO_OFFSET(bb, slot, expr, type, member) {\
        IRTemp tmp = newIRTemp(bb->tyenv, IRTYPE_PTR);\
        IRExpr *offsetExpr = mkIRExpr_HWord((HWord)offsetof(type, member));\
        addStmtToIRSB(bb,\
                      IRStmt_WrTmp(tmp,\
                                   IRExpr_Binop(IOP_ADD_PTR,\
                                                IRExpr_RdTmp(slot), offsetExpr)));\
        addStmtToIRSB(bb,\
                      IRStmt_Store(ENDNESS,\
                                   IRExpr_RdTmp(tmp),\
                                   expr));\
    }

static IRTemp gnInstrumentEvent_Instr(IRSB *bb, GN_(InstrEvent) *ev,
                                      IRTemp slot, IRExpr *slotSize, IRType tyW)
{
    /* slot.tag <- instr tag */
    GN_STORE_CONST_TO_OFFSET(bb, slot, SGL_CXT_TAG, SglEvVariant, tag);

    /* slot.cxt.type <- instr */
    GN_STORE_CONST_TO_OFFSET(bb, slot, SGLPRIM_CXT_INSTR, SglEvVariant, cxt.type);

    /* slot.cxt.id <- iaddr */
    GN_STORE_CONST_TO_OFFSET(bb, slot, ev->id, SglEvVariant, cxt.id);

    return incrSlot(bb, slot, slotSize, tyW);
}


static IRTemp gnInstrumentEvent_Compute(IRSB *bb, GN_(ComputeEvent) *ev,
                                        IRTemp slot, IRExpr *slotSize, IRType tyW)
{
    /* slot.tag <- comp tag */
    GN_STORE_CONST_TO_OFFSET(bb, slot, SGL_COMP_TAG, SglEvVariant, tag);

    /* slot.comp.type <- iop/flop/simd */
    GN_STORE_CONST_TO_OFFSET(bb, slot, ev->type, SglEvVariant, comp.type);

    /* slot.comp.arity <- comp arity */
    GN_STORE_CONST_TO_OFFSET(bb, slot, ev->arity, SglEvVariant, comp.arity);

    return incrSlot(bb, slot, slotSize, tyW);
}


static IRTemp gnInstrumentEvent_Memory(IRSB *bb, GN_(MemoryEvent) *ev,
                                       IRTemp slot, IRExpr *slotSize, IRType tyW)
{
    /* slot.tag <- mem tag */
    GN_STORE_CONST_TO_OFFSET(bb, slot, SGL_MEM_TAG, SglEvVariant, tag);

    /* slot.mem.type <- load/store */
    GN_STORE_CONST_TO_OFFSET(bb, slot, ev->load ? SGLPRIM_MEM_LOAD : SGLPRIM_MEM_STORE,
                             SglEvVariant, mem.type);

    /* slot.mem.size <- access size */
    GN_STORE_CONST_TO_OFFSET(bb, slot, ev->size, SglEvVariant, mem.size);

    /* slot.mem.addr <- aexpr */
    GN_STORE_EXPR_TO_OFFSET(bb, slot, ev->aexpr, SglEvVariant, mem.begin_addr);

    IRTemp newSlot;

    if (ev->guarded == True) {
        /* only increment the slot if (guard == True) */
        newSlot = newIRTemp(bb->tyenv, tyW);
        IRExpr *ite = IRExpr_ITE(ev->guard,
                                 IRExpr_Binop(IOP_ADD_PTR, IRExpr_RdTmp(slot), slotSize),
                                 IRExpr_RdTmp(slot));
        addStmtToIRSB(bb,
                      IRStmt_WrTmp(newSlot, ite));
    }
    else {
        newSlot = incrSlot(bb, slot, slotSize, tyW);
    }

    return newSlot;
}


static void gnInstrument_JmpsPassed(IRSB *bb, Int jmp)
{
    /* GN_(lastJmpsPassed) = jmp */
    GN_ASSERT(sizeof(GN_(lastJmpsPassed)) == 4);
    IRExpr *jmpExpr = IRExpr_Const(IRConst_U32(jmp));
    IRExpr *addr = mkIRExpr_HWord((HWord)&GN_(lastJmpsPassed));
    addStmtToIRSB(bb,
                  IRStmt_Store(ENDNESS, addr, jmpExpr));
}


static void gnInstrument_skipIfEventGenDisabled(IRSB *bb, IRConst *dst, IRJumpKind ijk)
{
    if (!(GN_(clo).gen_sync == True ||
          GN_(clo).start_collect_func != NULL ||
          GN_(clo).stop_collect_func != NULL))
        return;

    GN_DEBUGIF(6) {
        HChar jstr[16];
        switch(ijk) {
        case Ijk_Ret:
            VG_(sprintf)(jstr, "Return");
            break;
        case Ijk_Call:
            VG_(sprintf)(jstr, "Call");
            break;
        case Ijk_Boring:
            VG_(sprintf)(jstr, "Boring");
            break;
        default:
            VG_(sprintf)(jstr, "Other");
        }
        VG_(printf)("SkipEventGen: Inserting Conditional %s to 0x%llX\n", jstr, dst->Ico.U64);
    }

    /* if event generation is disabled, then jump to the exit instruction */

    /* assumes that a Bool is 8 bits */
    IRTemp enabledTmp = newIRTemp(bb->tyenv, Ity_I8);
    IRExpr *enabledLoad = IRExpr_Load(ENDNESS, Ity_I8,
                                mkIRExpr_HWord((HWord)&GN_(EventGenerationEnabled)));
    IRTemp disabledTmp = newIRTemp(bb->tyenv, Ity_I1);

    /* tmp1 <- EventGenerationEnabled */
    addStmtToIRSB(bb,
                  IRStmt_WrTmp(enabledTmp, enabledLoad));

    /* tmp2 <- (tmp1 == 0) */
    addStmtToIRSB(bb,
                  IRStmt_WrTmp(disabledTmp,
                               IRExpr_Binop(Iop_CmpEQ8,
                                            IRExpr_RdTmp(enabledTmp),
                                            IRExpr_Const(IRConst_U8(0)))));

    /* if (tmp2) then jump to next instr */
    addStmtToIRSB(bb,
                  IRStmt_Exit(IRExpr_RdTmp(disabledTmp),
                              ijk, dst, OFFB_RIP));
}


static void gnInstrument_EventCapture(IRSB *nbb, IRType tyW, UInt eventsToFlush)
{
    /* make sure there's enough space in the current buffer,
     * or get a new buffer */
    gnReserveEventsInBuffer(nbb, tyW, eventsToFlush);

    /* Initialize the event slot temporary
     * tmp <- current event slot */
    IRTemp slotTmp = newIRTemp(nbb->tyenv, tyW);
    IRExpr *slotPtr = mkIRExpr_HWord((HWord)&GN_(currEv));
    addStmtToIRSB(nbb,
                  IRStmt_WrTmp(slotTmp,
                               IRExpr_Load(ENDNESS, tyW,
                                           slotPtr)));

    /* now we have enough event slots in the buffer,
     * load in the buffer pointer and flush events to buffer */
    IRExpr *slotSize = mkIRExpr_HWord((HWord)sizeof(SglEvVariant));
    for (UInt i=0; i<eventsToFlush; ++i) {
        /* fill in event specific attributes and get the next slot
         * ML: We compute the next slot in each event instrumentation
         * because the slot does not always increment by 1, such as
         * in a conditional memory event */
        switch(GN_(EvBuffer)[i].tag) {
        case GN_INSTR_EV:
            slotTmp = gnInstrumentEvent_Instr(nbb, &GN_(EvBuffer)[i].instr,
                                              slotTmp, slotSize, tyW);
            break;
        case GN_COMPUTE_EV:
            slotTmp = gnInstrumentEvent_Compute(nbb, &GN_(EvBuffer)[i].comp,
                                                slotTmp, slotSize, tyW);
            break;
        case GN_MEMORY_EV:
            slotTmp = gnInstrumentEvent_Memory(nbb, &GN_(EvBuffer)[i].mem,
                                               slotTmp, slotSize, tyW);
            break;
        default:
            tl_assert(0);
            break;
        }
    }

    /* store the new buffer length */
    addStmtToIRSB(nbb,
                  IRStmt_Store(ENDNESS,
                               slotPtr, IRExpr_RdTmp(slotTmp)));

    /* update the buffer used var */
    GN_ASSERT(sizeof(*GN_(usedEv)) == sizeofIRType(tyW));
    IRTemp usedPtrTmp = newIRTemp(nbb->tyenv, tyW);
    IRExpr *usedPtr = mkIRExpr_HWord((HWord)&GN_(usedEv));
    addStmtToIRSB(nbb,
                  IRStmt_WrTmp(usedPtrTmp,
                               IRExpr_Load(ENDNESS, tyW,
                                           usedPtr)));
    IRTemp usedTmp = newIRTemp(nbb->tyenv, tyW);
    addStmtToIRSB(nbb,
                  IRStmt_WrTmp(usedTmp,
                               IRExpr_Load(ENDNESS, tyW,
                                           IRExpr_RdTmp(usedPtrTmp))));
    IRTemp newUsedTmp = newIRTemp(nbb->tyenv, tyW);
    IRExpr *eventsAdded = mkIRExpr_HWord((HWord)eventsToFlush);
    addStmtToIRSB(nbb,
                  IRStmt_WrTmp(newUsedTmp,
                               IRExpr_Binop(IOP_ADD_PTR,
                                            IRExpr_RdTmp(usedTmp), eventsAdded)));
    addStmtToIRSB(nbb,
                  IRStmt_Store(ENDNESS,
                               IRExpr_RdTmp(usedPtrTmp),
                               IRExpr_RdTmp(newUsedTmp)));
}


void GN_(flushEvents)(BBState *bbState, Int prev_flush, GN_(Flush) flushType)
{
    GN_DEBUGIF(2) {
        GN_(printTabs)(1);
        VG_(printf)("Flushing Events\n");
    }

    IRSB *const nbb = bbState->nbb;
    IRSB *const obb = bbState->obb;

    Int flush_from = prev_flush + 1;
    Int flush_to;
    IRConst *skip_dst;
    IRJumpKind skip_jk;

    switch(flushType.tag) {
    case GN_FLUSH_EXIT_ST:
        flush_to = flushType.exit_imark_idx;
        skip_dst = IRCONST_PTR(obb->stmts[flushType.exit_imark_idx]->Ist.IMark.addr);
        skip_jk = Ijk_Boring;
        break;
    case GN_FLUSH_BB_END:
        flush_to = obb->stmts_used;
        GN_ASSERT(obb->next->tag == Iex_Const);
        skip_dst = IRCONST_PTR(obb->next->Iex.Const.con->Ico.U64);

        /* ideally we would use the jumpkind from the end of the basic block
         * (obb->jumpkind), but VEX doesn't allow us to insert exits with
         * Ijk_Call/Ret, so we unconditionally set it to Ijk_Boring and let our
         * callstack handler determine whether it was a Call or Return */
        skip_jk = Ijk_Boring;
        break;
    default:
        tl_assert(0);
    }

    if (flush_from == flush_to) {
        /* Should only happen when the last statement in the BB is an exit.
         * In that case there is no need to flush again because there are no
         * captured events between the last side-exit and the last unconditional
         * exit */
        return;
    }

    /* Perform the instrumentation for the event capture flush.
     *
     * - insert IR from obb[begin] to obb[end] into nbb
     * - insert conditional jump over event-capture instrumentation to skip_to
     * - insert event capture instrumentation
     */

    for (Int i = flush_from; i < flush_to; ++i)
        addStmtToIRSB(nbb, obb->stmts[i]);
    gnInstrument_skipIfEventGenDisabled(nbb, skip_dst, skip_jk);
    gnInstrument_EventCapture(nbb, bbState->hWordTy, bbState->eventsToFlush);

    /* add the exit after instrumentation */
    if (flushType.tag == GN_FLUSH_EXIT_ST)
        addStmtToIRSB(nbb, obb->stmts[flush_to]);

    /* any extra client instrumentation
     * (anything not being sent to the event analysis frontend) */
    if (GN_(clo).gen_fn == True)
        gnInstrument_JmpsPassed(nbb, bbState->jmpsPassed); // set which jump this flush is at
    bbState->jmpsPassed++;

    /* reset events for next flush */
    if (GN_(clo).bbinfo_needed == True)
        bbState->bbInfo->eventsTotal += bbState->eventsToFlush;
    bbState->eventsToFlush = 0;
}


void GN_(flush_Sync)(SyncType type, SyncID *data, UInt args)
{
    /* Synchronization events are flushed immediately and not queued in a buffer.
     * Synchronization events are not generated in BB instrumentation.
     * Instead, they are  generated at either a context switch,
     * or a thread API call/return. */

    GN_ASSERT(GN_(currEv) < GN_(endEv));

    /* add event */
    GN_ASSERT(args <= MAX_SYNC_DATA && args > 0);
    SglEvVariant *slot = GN_(currEv);
    slot->tag = SGL_SYNC_TAG;
    slot->sync.type = type;

    UInt i=0;
    for (; i<args; ++i)
        slot->sync.data[i] = data[i];
    for (; i<MAX_SYNC_DATA; ++i)
        slot->sync.data[i] = UNUSED_SYNC_DATA;

    /* increment event slot */
    ++GN_(currEv);
    ++*GN_(usedEv);
    if (GN_(currEv) == GN_(endEv))
        GN_(flushCurrAndSetNextBuffer)();

    GN_DEBUGIF(6) {
        HChar str[16];
        GN_(sprint_SyncType)(str, type);
        VG_(printf)("SyncEvent: %s; SyncData0: 0x%lu; SyncData1: 0x%lu\n",
                    str, data[0], data[1]);
    }
}


void GN_(flush_FnEnter)(const HChar *fnname)
{
    GN_DEBUG(6, "Fn Enter: %s\n", fnname);
}


void GN_(flush_FnExit)(const HChar *fnname)
{
    GN_DEBUG(6, "Fn Exit : %s\n", fnname);
}


void GN_(updateEventGeneration)(void)
{
    if (GN_(afterStartFunc) == True &&
            GN_(afterEndFunc) == False &&
            GN_(isInSyncCall)() == False) {
        GN_(EventGenerationEnabled) = True;
    }
    else {
        GN_(EventGenerationEnabled) = False;
    }
}

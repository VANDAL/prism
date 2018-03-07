#include "gn_bb.h"
#include "gn_fn.h"
#include "gn_clo.h"
#include "gn_debug.h"


BBTable GN_(allBBs);
BBInfo *GN_(lastBB);
ULong GN_(bbExecutions);
ULong GN_(bbSeen);


//-------------------------------------------------------------------------------------------------
/** Helper function definitions **/

static inline Int bbHashIdx(ObjNode *obj, PtrdiffT offset, UInt size)
{
    return (((Addr)obj + offset) % size);
}


static void resizeBBTable(void)
{
    /* Double size
     * Requires rehashing all BBs in the current table */

    UInt conflicts1 = 0, conflicts2 = 0; // for hash table stats

    UInt newcap = 2 * GN_(allBBs).capacity + 3;
    BBInfo **newtable = VG_(malloc)("gn.bb.resize.1", newcap * sizeof(BBInfo*));

    for (UInt i=0; i<newcap; ++i)
        newtable[i] = NULL;

    for (UInt i=0; i<GN_(allBBs).capacity; ++i) {

        if (GN_(allBBs).table[i] == NULL)
            continue;

        BBInfo *curr = GN_(allBBs).table[i];
        while (curr != NULL) {
            BBInfo *next = curr->next;
            UInt newidx = bbHashIdx(curr->obj, curr->offset, newcap);

            curr->next = newtable[newidx];
            newtable[newidx] = curr;

            /* collect some hash table stats */
            if (curr->next) {
                conflicts1++;
                if (curr->next->next) {
                    conflicts2++;
                }
            }

            curr = next;
        }
    }

    VG_(free)(GN_(allBBs).table);
    GN_(allBBs).capacity = newcap;
    GN_(allBBs).table = newtable;
}


static BBInfo* newBB(ObjNode *obj, PtrdiffT offset)
{
    BBInfo *bb;

    GN_(allBBs).entries++;

    /* Use Callgrind's hash implementation
     * resize if filled more than 80% */
    if (10 * GN_(allBBs).entries / GN_(allBBs).capacity > 8)
        resizeBBTable();

    bb = VG_(malloc)("gn.bb.newbb.1", sizeof(BBInfo));

    bb->offset   = offset;
    bb->obj      = obj;
    bb->sectKind = VG_(DebugInfo_sect_kind)(NULL, offset + obj->offset);

    // To be set during instrumentation
    bb->eventsTotal     = 0;
    bb->jmpCount        = 0;
    bb->jmps            = NULL;
    bb->lruToJmp        = NULL;
    bb->lruFromJmp      = NULL;
    bb->lastJmpInverted = False;

    bb->uid = GN_(bbSeen)++;

    /* insert into hash */
    UInt idx = bbHashIdx(obj, offset, GN_(allBBs).capacity);
    bb->next = GN_(allBBs).table[idx];
    GN_(allBBs).table[idx] = bb;

    /* set up function info */
    bb->fn = NULL;
    bb->isFnEntry = False;
    GN_(getFnNode)(bb);

    return bb;
}


//-------------------------------------------------------------------------------------------------
/** External function definitions **/

BBInfo* GN_(getBBInfo)(Addr addr)
{
    BBInfo *bb;

    ObjNode *obj = GN_(getObjNodeByAddr)(addr);
    PtrdiffT offset = addr - obj->offset;

    Int idx = bbHashIdx(obj, offset, GN_(allBBs).capacity);
    bb = GN_(allBBs).table[idx];

    while (bb) {
        if (bb->obj == obj && bb->offset == offset)
            break;
        bb = bb->next;
    }

    if (bb == NULL)
        bb = newBB(obj, offset);

    return bb;
}


Int GN_(initBBState)(BBState *bbState, IRSB *obb, IRType hWordTy)
{
    Int i = 0;

    bbState->obb = obb;
    bbState->eventsToFlush = 0;

    // Set up NBB for instrumentation
    bbState->nbb = deepCopyIRSBExceptStmts(obb);
    bbState->hWordTy = hWordTy;

    // Copy verbatim any IR preamble preceding the first IMark
    while (i < obb->stmts_used && obb->stmts[i]->tag != Ist_IMark) {
        addStmtToIRSB( bbState->nbb, obb->stmts[i] );
        i++;
    }

    // Get the first statement
    GN_ASSERT(obb->stmts_used > 0);
    GN_ASSERT(i < obb->stmts_used);
    GN_ASSERT(Ist_IMark == obb->stmts[i]->tag);

    // Any extra state needed
    bbState->jmpsPassed = 0;
    if (GN_(clo).bbinfo_needed == True) {
        const IRStmt *st = obb->stmts[i];
        Addr origAddr = st->Ist.IMark.addr + st->Ist.IMark.delta;
        bbState->bbInfo = GN_(getBBInfo)(origAddr);
    }
    else {
        bbState->bbInfo = NULL;
    }

    /* return index of first instruction IR */
    return i;
}


void GN_(initAllBBsTable)()
{
    /* Once again, taken from Callgrind */
    GN_(allBBs).capacity = 8347;
    GN_(allBBs).entries = 0;
    GN_(allBBs).table = VG_(malloc)("gn.bb.inittable.1",
                                    GN_(allBBs).capacity * sizeof(BBInfo*));

    for (UInt i=0; i<GN_(allBBs).capacity; ++i)
        GN_(allBBs).table[i] = NULL;
}


void GN_(initBB)()
{
    GN_(initAllBBsTable)();
    GN_(lastBB) = NULL;
    GN_(bbExecutions) = 0;
    GN_(bbSeen) = 0;
}

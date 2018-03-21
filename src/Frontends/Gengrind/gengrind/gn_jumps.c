#include "gn_jumps.h"
#include "gn_bb.h"
#include "gn_debug.h"


UInt GN_(lastJmpsPassed);
static JumpTable allJns;


//-------------------------------------------------------------------------------------------------
/** Helper function definitions **/

static inline UInt jnHashIdx(BBInfo *from, UInt jmp, BBInfo *to, UInt capacity)
{
    return (UInt) ( (UWord)from + 7 * (UWord)to + 13 * jmp) % capacity;
}


#define N_JN_INITIAL_ENTRIES (4437)
static void initJumpTable(JumpTable *jt)
{
    GN_ASSERT(jt != NULL);

    jt->capacity = N_JN_INITIAL_ENTRIES;
    jt->entries = 0;
    jt->table = VG_(malloc)("gn.jumps.initjt.1", jt->capacity * sizeof(JumpNode*));
    jt->spontaneous = NULL;

    for (UInt i=0; i<jt->capacity; ++i)
        jt->table[i] = NULL;
}


static void resizeJumpTable(JumpTable *jt)
{
    UInt newcap = 2 * jt->capacity + 3;
    JumpNode **newtable = VG_(malloc)("gn.jumps.resize.1", newcap * sizeof(JumpNode*));

    for (UInt i=0; i<newcap; ++i)
        newtable[i] = NULL;

    for (UInt i=0; i<jt->capacity; ++i) {

        JumpNode *jn = jt->table[i];
        if (jn == NULL)
            continue;
        while (jn != NULL) {
            JumpNode *next = jn->next_hash;
            UInt newidx = jnHashIdx(jn->from, jn->jmp, jn->to, newcap);
            jn->next_hash = newtable[newidx];
            newtable[newidx] = jn;
            jn = next;
        }
    }

    VG_(free)(jt->table);
    jt->table = newtable;
    jt->capacity = newcap;
}


static JumpNode* newJumpNode(BBInfo *from, UInt jmp, BBInfo *to)
{
    JumpNode *jn;

    allJns.entries++;

    /* Use Callgrind's hash implementation
     * resize if filled more than 80% */
    if (10 * allJns.entries / allJns.capacity > 8)
        resizeJumpTable(&allJns);

    jn = VG_(malloc)("gn.jumps.newjn.1", sizeof(JumpNode));

    jn->from = from;
    jn->jmp = jmp;
    jn->to = to;
    jn->jmpkind = jk_Call; // ML: why is this?

    jn->next_from = allJns.spontaneous;
    allJns.spontaneous = jn;

    UInt idx = jnHashIdx(from, jmp, to, allJns.capacity);
    jn->next_hash = allJns.table[idx];
    allJns.table[idx] = jn;

    return jn;
}


//-------------------------------------------------------------------------------------------------
/** External function definitions **/

JumpNode* GN_(getJumpNode)(BBInfo *from, UInt jmp, BBInfo *to)
{
    JumpNode *jn;

    if ((jn = to->lruToJmp) && (jn->from == from) && (jn->jmp = jmp)) {
        GN_ASSERT(jn->to == to);
    }
    else if ((jn = from->lruFromJmp) && (jn->to == to) && (jn->jmp == jmp)) {
        GN_ASSERT(jn->from == from);
    }
    else {
        UInt idx = jnHashIdx(from, jmp, to, allJns.capacity);
        jn = allJns.table[idx];

        while(jn != NULL) {
            if ((jn->from == from) && (jn->jmp == jmp) && (jn->to == to))
                break;
            jn = jn->next_hash;
        }

        if (jn == NULL)
            jn = newJumpNode(from, jmp, to);

        from->lruFromJmp = jn;
        to->lruToJmp = jn;
    }

    return jn;
}


void GN_(initJumpTable)()
{
    initJumpTable(&allJns);
}


static HChar jmpstr[32];
const HChar* GN_(getJumpStr)(GnJumpKind jk, Bool isConditional)
{
    if (isConditional == True)
        VG_(sprintf)(jmpstr, "Cond-");
    else
        jmpstr[0] = '\0';

    switch(jk) {
    case jk_None:
        VG_(sprintf)(jmpstr + VG_(strlen)(jmpstr), "Fall-through");
        break;
    case jk_Call:
        VG_(sprintf)(jmpstr + VG_(strlen)(jmpstr), "Call");
        break;
    case jk_Return:
        VG_(sprintf)(jmpstr + VG_(strlen)(jmpstr), "Return");
        break;
    case jk_Jump:
        VG_(sprintf)(jmpstr + VG_(strlen)(jmpstr), "Jump");
        break;
    case jk_Other:
        VG_(sprintf)(jmpstr + VG_(strlen)(jmpstr), "Other");
        break;
    default:
        tl_assert(0);
    }

    return jmpstr;
}

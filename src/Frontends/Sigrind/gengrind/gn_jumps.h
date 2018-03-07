#ifndef GN_JUMPS_H
#define GN_JUMPS_H

#include "gn.h"


//-------------------------------------------------------------------------------------------------
/** Jump related type definitions **/

enum _GnJumpKind {
    /* The types of control flow changes that can happen between
     * execution of two BBs in a thread.
     */

    jk_None = 0,   /* no explicit change by a guest instruction */
    jk_Jump,       /* regular jump */
    jk_Call,
    jk_Return,
    jk_CondJump,   /* conditional jump taken (only used as JumpNode type) */
    jk_Other,
};


struct _JumpNode {
    /* JmpCall node
     * for subroutine call (from->bb->jmp_addr => to->bb->addr)
     *
     * Each BB has at most one CALL instruction. The list of JumpNode
     * from this call is a pointer to the list head (stored in BB_node),
     * and <next_from> in the JumpNode struct.
     *
     * For fast lookup, JumpNodes are reachable with a hash table, keyed by
     * the (from_bb_node,to) pair. <next_hash> is used for the JumpNode chain
     * of one hash table entry.
     */ 

    GnJumpKind jmpkind;  /* jk_Call, jk_Jump, jk_CondJump */
    JumpNode* next_hash; /* for hash entry chain */
    JumpNode* next_from; /* next JumpNode from a BB */
    BBInfo *from, *to;   /* call arc from/to this BB */
    UInt jmp;            /* jump no. in source */
};


struct _JumpTable {
    UInt capacity;
    UInt entries;
    JumpNode **table;
    JumpNode *spontaneous;
};


//-------------------------------------------------------------------------------------------------
/** Jump related function definitions **/

JumpNode* GN_(getJumpNode)(BBInfo *from, UInt jmp, BBInfo *to);
void GN_(initJumpTable)(void);
const HChar* GN_(getJumpStr)(GnJumpKind jk, Bool isConditional);

#endif

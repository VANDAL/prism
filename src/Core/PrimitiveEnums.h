#ifndef SGL_PRIM_ENUM_H
#define SGL_PRIM_ENUM_H

#ifndef __cplusplus
typedef enum CompArityEnum CompArityEnum;
typedef enum CompCostTypeEnum CompCostTypeEnum;
typedef enum CompCostOpEnum CompCostOpEnum;
typedef enum MemTypeEnum MemTypeEnum;
typedef enum CFTypeEnum CFTypeEnum;
typedef enum CxtTypeEnum CxtTypeEnum;
typedef enum SyncTypeEnum SyncTypeEnum;
typedef enum EvTagEnum EvTagEnum;
#endif

//-----------------------------------------------------------------------------
/**   Memory   **/
enum MemTypeEnum
{
    SGLPRIM_MEM_TYPE_UNDEF = 0,
    SGLPRIM_MEM_LOAD,
    SGLPRIM_MEM_STORE
};


//-----------------------------------------------------------------------------
/**  Compute   **/
enum CompCostTypeEnum
{
    SGLPRIM_COMP_TYPE_UNDEF = 0,
    SGLPRIM_COMP_IOP,
    SGLPRIM_COMP_FLOP
};

enum CompArityEnum
{
    SGLPRIM_COMP_ARITY_UNDEF = 0,
    SGLPRIM_COMP_NULLARY,
    SGLPRIM_COMP_UNARY,
    SGLPRIM_COMP_BINARY,
    SGLPRIM_COMP_TERNARY,
    SGLPRIM_COMP_QUARTERNARY
};

enum CompCostOpEnum
{
    SGLPRIM_COMP_OP_UNDEF = 0,
    SGLPRIM_COMP_ADD,
    SGLPRIM_COMP_SUB,
    SGLPRIM_COMP_MULT,
    SGLPRIM_COMP_DIV,
    SGLPRIM_COMP_SHFT,
    SGLPRIM_COMP_MOV
};


//-----------------------------------------------------------------------------
/** Control Flow **/
enum CFTypeEnum
{
    SGLPRIM_CF_UNDEF = 0,
    SGLPRIM_CF_JUMP,
    SGLPRIM_CF_BRANCH_CND,
};


//-----------------------------------------------------------------------------
/**  Context  **/
enum CxtTypeEnum
{
    SGLPRIM_CXT_UNDEF = 0,
    SGLPRIM_CXT_INSTR,
    SGLPRIM_CXT_BB,
    SGLPRIM_CXT_FUNC_ENTER,
    SGLPRIM_CXT_FUNC_EXIT,
    SGLPRIM_CXT_THREAD,
};


//-----------------------------------------------------------------------------
/** Synchronization **/
enum SyncTypeEnum
{
    /* TODO(someday) Many of these are specific
     * to SynchroTrace. Can we simplify
     * SynchroTrace to take a subset? */

    SGLPRIM_SYNC_UNDEF = 0,
    SGLPRIM_SYNC_CREATE,
    SGLPRIM_SYNC_JOIN,
    SGLPRIM_SYNC_BARRIER,
    SGLPRIM_SYNC_SYNC,
    SGLPRIM_SYNC_SWAP,

    /* SynchroTrace specific */
    SGLPRIM_SYNC_LOCK,
    SGLPRIM_SYNC_UNLOCK,
    SGLPRIM_SYNC_CONDWAIT,
    SGLPRIM_SYNC_CONDSIG,
    SGLPRIM_SYNC_CONDBROAD,
    SGLPRIM_SYNC_SPINLOCK,
    SGLPRIM_SYNC_SPINUNLOCK,
};


//-----------------------------------------------------------------------------
enum EvTagEnum
{
    SGL_EV_UNDEF = 0,
    SGL_MEM_TAG,
    SGL_COMP_TAG,
    SGL_CF_TAG,
    SGL_CXT_TAG,
    SGL_SYNC_TAG,
};

#endif //SGL_PRIM_ENUM_H

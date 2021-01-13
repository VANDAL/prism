#ifndef SIGIL2_PRIM_ENUM_H
#define SIGIL2_PRIM_ENUM_H

#ifndef __cplusplus
typedef enum CompArityEnum CompArityEnum;
typedef enum CompCostTypeEnum CompCostTypeEnum;
typedef enum CompCostOpEnum CompCostOpEnum;
typedef enum MemTypeEnum MemTypeEnum;
typedef enum CFTypeEnum CFTypeEnum;
typedef enum CxtTypeEnum CxtTypeEnum;
typedef enum SyncTypeEnum SyncTypeEnum;
typedef enum EventTypeEnum EventTypeEnum;
#endif

//-----------------------------------------------------------------------------
/**   Memory   **/
enum MemTypeEnum
{
    PRISM_MEM_TYPE_UNDEF = 0,
    PRISM_MEM_LOAD,
    PRISM_MEM_STORE
};
enum MemSizeEnum
{
    // in bits
    PRISM_MEM_8,
    PRISM_MEM_16,
    PRISM_MEM_32,
    PRISM_MEM_64,
    PRISM_MEM_128,
    PRISM_MEM_256,
    PRISM_MEM_512,
    PRISM_MEM_1024,
    // IMPORTANT: DO NOT ADD MORE w/o substantial changes
    // The bit-layout of events only has 3-bits dedicated to size (8 options)
    PRISM_MEM_SIZES, 
};


//-----------------------------------------------------------------------------
/**  Compute   **/
enum CompCostTypeEnum
{
    PRISM_COMP_TYPE_UNDEF = 0,
    PRISM_COMP_IOP,
    PRISM_COMP_FLOP
};

enum CompArityEnum
{
    PRISM_COMP_UNARY = 0,
    PRISM_COMP_BINARY,
    PRISM_COMP_TERNARY,
    PRISM_COMP_QUARTERNARY
};

enum CompCostOpEnum
{
    PRISM_COMP_OP_UNDEF = 0,
    PRISM_COMP_ADD,
    PRISM_COMP_SUB,
    PRISM_COMP_MULT,
    PRISM_COMP_DIV,
    PRISM_COMP_SHFT,
    PRISM_COMP_MOV
};



//-----------------------------------------------------------------------------
/** Control Flow **/
enum CFTypeEnum
{
    PRISM_CF_UNDEF = 0,
    PRISM_CF_JUMP,
    PRISM_CF_BRANCH_CND,
};


//-----------------------------------------------------------------------------
/**  Context  **/
enum CxtTypeEnum
{
    PRISM_CXT_UNDEF = 0,
    PRISM_CXT_INSTR,
    PRISM_CXT_BB,
    PRISM_CXT_FUNC_ENTER,
    PRISM_CXT_FUNC_EXIT,
    PRISM_CXT_THREAD,
    PRISM_CXT_ML_NET_ENTER,
    PRISM_CXT_ML_NET_EXIT,
    PRISM_CXT_ML_OP_ENTER,
    PRISM_CXT_ML_OP_EXIT,
};


//-----------------------------------------------------------------------------
/** Synchronization **/
enum SyncTypeEnum
{
    /* TODO(someday) Many of these are specific
     * to SynchroTrace. Can we simplify
     * SynchroTrace to take a subset? */

    PRISM_SYNC_UNDEF = 0,
    PRISM_SYNC_CREATE,
    PRISM_SYNC_JOIN,
    PRISM_SYNC_BARRIER,
    PRISM_SYNC_SYNC,
    PRISM_SYNC_SWAP,

    /* SynchroTrace specific */
    PRISM_SYNC_LOCK,
    PRISM_SYNC_UNLOCK,
    PRISM_SYNC_CONDWAIT,
    PRISM_SYNC_CONDSIG,
    PRISM_SYNC_CONDBROAD,
    PRISM_SYNC_SPINLOCK,
    PRISM_SYNC_SPINUNLOCK,
};


//-----------------------------------------------------------------------------
enum EventTypeEnum {
    PRISM_EVENTTYPE_MEM = 0,
    PRISM_EVENTTYPE_COMP,
    PRISM_EVENTTYPE_SYNC,
    PRISM_EVENTTYPE_CXT,
    PRISM_EVENTTYPE_CF,
    PRISM_EVENTTYPE_CFG,
    PRISM_EVENTTYPE_END,
};

#endif //PRISM_PRIM_ENUM_H

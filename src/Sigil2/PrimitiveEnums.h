#ifndef SGL_PRIM_ENUM_H
#define SGL_PRIM_ENUM_H

////////////////
/*  Compute   */
////////////////
enum CompCostType
{
	SGLPRIM_COMP_TYPE_UNDEF = 0,
	SGLPRIM_COMP_IOP,
	SGLPRIM_COMP_FLOP
};

enum CompArity
{
	SGLPRIM_COMP_ARITY_UNDEF = 0,
	SGLPRIM_COMP_NULLARY,
	SGLPRIM_COMP_UNARY,
	SGLPRIM_COMP_BINARY,
	SGLPRIM_COMP_TERNARY,
	SGLPRIM_COMP_QUARTERNARY
};

enum CompCostOp
{
	SGLPRIM_COMP_OP_UNDEF = 0,
	SGLPRIM_COMP_ADD,
	SGLPRIM_COMP_SUB,
	SGLPRIM_COMP_MULT,
	SGLPRIM_COMP_DIV,
	SGLPRIM_COMP_SHFT,
	SGLPRIM_COMP_MOV
};


////////////////
/*   Memory   */
////////////////
enum MemType
{
	SGLPRIM_MEM_TYPE_UNDEF = 0,
	SGLPRIM_MEM_LOAD,
	SGLPRIM_MEM_STORE
};


//////////////////
/* Control Flow */
//////////////////
enum CFType
{
	SGLPRIM_CF_UNDEF = 0,
	SGLPRIM_CF_JUMP,
	SGLPRIM_CF_BRANCH_CND
};


///////////////
/*  Context  */
///////////////
enum CxtType
{
	SGLPRIM_CXT_UNDEF = 0,
	SGLPRIM_CXT_FUNC = 1,
	SGLPRIM_CXT_THREAD = 2 
	//instr, etc, unimplemented
};


/////////////////////
/* Synchronization */
/////////////////////
/* TODO Many of these are specific
 * to SynchroTrace. Can we simplify
 * SynchroTrace to take a subset? */
enum SyncType
{
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
	SGLPRIM_SYNC_SPINLOCK,
	SGLPRIM_SYNC_SPINUNLOCK
};

#ifndef __cpluscplus
typedef enum CompArity CompArity;
typedef enum CompCostType CompCostType;
typedef enum CompCostOp CompCostOp;

typedef enum MemType MemType;

typedef enum CFType CFType;

typedef enum CxtType CxtType;

typedef enum SyncType SyncType;
#endif

#endif //SGL_PRIM_ENUM_H

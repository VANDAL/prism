#ifndef SGL_PRIM_ENUM_H
#define SGL_PRIM_ENUM_H

////////////////
/*  Compute   */
////////////////
enum CompCostType
{
	COMP_TYPE_UNDEF = 0,
	COMP_IOP,
	COMP_FLOP
};

enum CompArity
{
	COMP_ARITY_UNDEF = 0,
	COMP_NULLARY,
	COMP_UNARY,
	COMP_BINARY,
	COMP_TERNARY,
	COMP_QUARTERNARY
};

enum CompCostOp
{
	COMP_OP_UNDEF = 0,
	COMP_ADD,
	COMP_SUB,
	COMP_MULT,
	COMP_DIV,
	COMP_SHFT,
	COMP_MOV
};


////////////////
/*   Memory   */
////////////////
enum MemType
{
	MEM_TYPE_UNDEF = 0,
	MEM_LOAD,
	MEM_STORE
};


//////////////////
/* Control Flow */
//////////////////
enum CFType
{
	CF_UNDEF = 0,
	CF_JUMP,
	CF_BRANCH_CND
};


///////////////
/*  Context  */
///////////////
enum CxtType
{
	CXT_UNDEF = 0,
	CXT_FUNC = 1,
	CXT_THREAD = 2 
	//instr, etc, unimplemented
};


/////////////////////
/* Synchronization */
/////////////////////
enum SyncType
{
	SYNC_UNDEF = 0,
	SYNC_CREATE,
	SYNC_JOIN,
	SYNC_BARRIER,
	SYNC_SYNC,
	SYNC_SWAP
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

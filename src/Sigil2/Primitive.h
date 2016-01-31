#ifndef SGL_PRIM_H
#define SGL_PRIM_H

/*
 * All Sigil primitives
 *
 * SglMemEv  -- a memory access event, 
 *              e.g. simple loads/stores, ...
 *
 * SglCompEv -- a compute event, 
 *              e.g. adds, floating points, SIMD, ...
 *
 * SglCFEv   -- a control flow event, //useful for microarchitecture studies, GPGPU, ...
 *              e.g. branches and jumps
 *
 * SglCxtEv  -- a scope marker, user is is responsible for freeing name data
 *              e.g. marking function entrance/exit, instruction boundaries, threads, ...
 *
 * SglSyncEv -- a synchronization event,
 *              e.g. create, join, sync, barrier, ...
 *
 * These primitives are created in the event generation front end, 
 * and passed to Sigil's event manager for further processing
 */

//TODO make types more portable
#include <stdint.h> 

#include "PrimitiveEnums.h"

typedef uint64_t Addr;
typedef uint32_t UInt;
typedef uint8_t UChar;

#ifdef __cpluscplus
extern "C" {
#else
typedef struct SglMemEv SglMemEv;
typedef struct SglCompEv SglCompEv;
typedef struct SglCFEv SglCFEv;
typedef struct SglCxtEv SglCxtEv;
typedef struct SglSyncEv SglSyncEv;
typedef struct BufferedSglEv BufferedSglEv;
#endif

struct SglMemEv
{
	MemType type;
	Addr    begin_addr;
	UInt    size; //bytes
	UChar   alignment; //TODO useful? Can calculate from address+size
};

struct SglCompEv
{
	CompCostType type;
	CompArity    arity;
	CompCostOp   op;
	UChar        size; //TODO rename?
};

//TODO unimplemented
struct SglCFEv
{
	CFType type;
};

struct SglCxtEv
{
	CxtType type;
	UChar   op; /* meaning changes based on
				   CxtType */

	char* name;
	UChar len;
};

struct SglSyncEv
{
	SyncType type;
	Addr id;
};

struct BufferedSglEv
{
	EvTag tag;
	union 
	{
		SglMemEv  mem_ev;
		SglCompEv comp_ev;
		SglCFEv   cf_ev;
		SglCxtEv  cxt_ev;
		SglSyncEv sync_ev;
	};
};

#ifdef __cpluscplus
}
#endif

#endif //_SGLPRIM_H_

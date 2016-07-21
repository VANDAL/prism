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

/* FIXME only 64-bit compatible right now */
typedef int64_t SLong;
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
        Addr    id;

        /* how to implement efficiently? */
        char *name;
        UChar len;
    };

    struct SglSyncEv
    {
        SyncType type;
        SLong id;
    };

    struct BufferedSglEv
    {
        EvTag tag;
        union
        {
            SglMemEv  mem;
            SglCompEv comp;
            SglCFEv   cf;
            SglCxtEv  cxt;
            SglSyncEv sync;
        };
    };

#ifdef __cpluscplus
}
#endif

#endif //_SGLPRIM_H_

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

#include <stdint.h>

#include "PrimitiveEnums.h"

typedef intptr_t  SyncID;
typedef uintptr_t Addr;
typedef uint32_t  ByteCount;

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
        MemType       type;
        Addr          begin_addr;
        ByteCount     size;
        unsigned char alignment; // TODO Is this useful? Can calculate from address+size
    };

    struct SglCompEv
    {
        CompCostType  type;
        CompArity     arity;
        CompCostOp    op;
        unsigned char size; // TODO rename?
    };

    /* XXX unimplemented */
    struct SglCFEv
    {
        CFType        type;
    };

    struct SglCxtEv
    {
        CxtType type;
        Addr    id;

        /* how to implement efficiently? */
        char         *name;
        unsigned char len;
    };

    struct SglSyncEv
    {
        SyncType      type;
        SyncID        id;
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

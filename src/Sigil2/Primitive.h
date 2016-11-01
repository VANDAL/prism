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
 *
 * XXX MDL20160814
 * These primitives are used both for presentation to a user-backend, and also
 * used for IPC between event generation frontends. Due to the pure amount and
 * variety of events transferred, each struct is packed to save memory space.
 * Aligned structs would not be of much benefit anyway, due to the way events
 * are serialized in shared memory. Otherwise, there is an overhead of up to 8x
 * in memory space (using unions and padding structs for alignment in arrays).
 */

#include <stdint.h>
#include "PrimitiveEnums.h"

#ifdef __cplusplus
extern "C" {
#else
typedef struct SglMemEv SglMemEv;
typedef struct SglCompEv SglCompEv;
typedef struct SglCFEv SglCFEv;
typedef struct SglCxtEv SglCxtEv;
typedef struct SglSyncEv SglSyncEv;
typedef struct BufferedSglEv BufferedSglEv;
#endif

    typedef uintptr_t Addr;
    typedef uint16_t ByteCount;
    typedef intptr_t SyncID;
    typedef uint8_t MemType;
    typedef uint8_t CompCostType;
    typedef uint8_t CompArity;
    typedef uint8_t CompCostOp;
    typedef uint8_t CFType;
    typedef uint8_t CxtType;
    typedef uint8_t SyncType;
    typedef uint8_t EvTag;

    struct SglMemEv
    {
        Addr          begin_addr;
        ByteCount     size;
        MemType       type;
    } __attribute__ ((__packed__));

    struct SglCompEv
    {
        CompCostType  type;
        CompArity     arity;
        CompCostOp    op;
        uint8_t       size;
    } __attribute__ ((__packed__));

    /* XXX unimplemented */
    struct SglCFEv
    {
        CFType        type;
    } __attribute__ ((__packed__));

    struct SglCxtEv
    {
        CxtType type;
        union
        {
            Addr id;
            struct
            {
                uint32_t idx;
                uint8_t  len; /* Implicit max length [256] */
            } __attribute__ ((__packed__));
        } __attribute__ ((__packed__));
    };

    struct SglSyncEv
    {
        SyncType      type;
        SyncID        id;
    } __attribute__ ((__packed__));

    struct BufferedSglEv
    {
        union
        {
            SglMemEv  mem;
            SglCompEv comp;
            SglCFEv   cf;
            SglCxtEv  cxt;
            SglSyncEv sync;
        };
        EvTag tag;
    } __attribute__ ((__packed__));

#ifdef __cplusplus
}
#endif

#endif //_SGLPRIM_H_

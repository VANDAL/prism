#ifndef PRISM_PRIM_H
#define PRISM_PRIM_H

/*
 * All Prism primitives
 *
 * PrismMemEv  -- a memory access event,
 *              e.g. simple loads/stores, ...
 *
 * PrismCompEv -- a compute event,
 *              e.g. adds, floating points, SIMD, ...
 *
 * PrismCFEv   -- a control flow event, //useful for microarchitecture studies, GPGPU, ...
 *              e.g. branches and jumps
 *
 * PrismCxtEv  -- a scope marker, user is is responsible for freeing name data
 *              e.g. marking function entrance/exit, instruction boundaries, threads, ...
 *
 * PrismSyncEv -- a synchronization event,
 *              e.g. create, join, sync, barrier, ...
 *
 * These primitives are created in the event generation front end,
 * and passed to Prism's event manager for further processing.
 *
 * They are EFFECTIVELY the raw data formats. Because a frontend
 * (e.g. instrumentation framework)
 * 1. may not support complex serialization libraries,
 * 2. and may have differing IPC capabilities,
 * this is as far as we define the events, and leave the rest of the
 * implementation up to the implementor to decide.
 *
 * These primitives are used for IPC between event generation frontends.
 * Due to the pure amount and variety of events transferred,
 * each struct is packed to save memory space.
 * Aligned structs do not have much performance benefit anyway, on x86,
 * due to the way events are streaming written to and read in shared memory.
 * Otherwise, without packing, there is an overhead of up to 8x in memory
 * footprint (using unions and padding structs for alignment in arrays).
 */

#include <stdint.h>
#include "PrimitiveEnums.h"

#ifdef __cplusplus
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>
extern "C" {
#else
typedef struct PrismMemEv PrismMemEv;
typedef struct PrismCompEv PrismCompEv;
typedef struct PrismCFEv PrismCFEv;
typedef struct PrismCxtEv PrismCxtEv;
typedef struct PrismSyncEv PrismSyncEv;
#endif

typedef uintptr_t addr_prism_type;
typedef uintptr_t SyncIdType;

/** Config event bit sizes */
#define PRISM_EVENT_TYPE_BITS ( 3 )
#define PRISM_CFG_EVTYPE_BITS ( 3 )
#define PRISM_CFG_RESERVED_SLOT1_BITS ( 2 )
#define PRISM_CFG_TOTAL_BITS ( 8 )
#define PRISM_MEM_TYPE_BITS ( 2 )
#define PRISM_MEM_SIZE_BITS ( 3 )
#define PRISM_MEM_ADDR_BITS ( 56 ) // TODO(someday) use all 32-bits for 32-bit archs
#define PRISM_MEM_ID_BITS ( 8 )
#define PRISM_COMP_FMT_BITS ( 2 )
#define PRISM_COMP_SIZE_BITS ( 3 )
#define PRISM_COMP_TYPE_BITS ( 6 )
#define PRISM_COMP_ARITY_BITS ( 2 )
#define PRISM_CXT_TYPE_BITS ( 5 )
#define PRISM_SYNC_TYPE_BITS ( 5 )

#define PRISM_BYTES_ADDR 7
#define PRISM_BYTES_CXT 1
#define PRISM_BYTES_MEM 1

// XXX I don't actually use these in practice, and instead just manually shift/set bits, so this is  mostly demonstrative.
// Bitfields aren't guaranteed to have a specific layout, so actual compatibility isn't tested.
// TODO(someday) add tests to check compatibility.
struct PrismCfgEvSlot_1 {
    // reserved, can be used for certain directives w/ metadata.
    // One example is growing and remapping a 'name' heap if we want to implement a shared memory heap
    // for sending function names via offsets into the heap.
    // This is in in contrast to what we do right now which is copying the entire C string each time.
    unsigned char : PRISM_CFG_RESERVED_SLOT1_BITS;
    unsigned char cfg_ty : PRISM_CFG_EVTYPE_BITS;
    unsigned char ev_ty : PRISM_EVENT_TYPE_BITS;
};

struct PrismCfgEvSlot_2 {
    unsigned char cfg : PRISM_CFG_TOTAL_BITS;
};
struct PrismMemEvSlot_1 {
    unsigned char mem_sz : PRISM_MEM_SIZE_BITS;
    unsigned char mem_ty : PRISM_MEM_TYPE_BITS;
    unsigned char ev_ty : PRISM_EVENT_TYPE_BITS;
};
struct PrismMemEvSlot_2 {
    /** TODO  check endianness order */
    uintptr_t : 8;
    uintptr_t mem_addr : PRISM_MEM_ADDR_BITS;
};
struct PrismCompEvSlot_1 {
    unsigned char op_sz : PRISM_COMP_SIZE_BITS;
    unsigned char op_fmt : PRISM_COMP_FMT_BITS;
    unsigned char ev_ty : PRISM_EVENT_TYPE_BITS;
};
struct PrismCompEvSlot_2 {
    unsigned char op_arity : PRISM_COMP_ARITY_BITS;
    unsigned char op_ty : PRISM_COMP_TYPE_BITS;
};
struct PrismCxtEvSlot_1 {
    unsigned char cxt_ty : PRISM_CXT_TYPE_BITS;
    unsigned char ev_ty : PRISM_EVENT_TYPE_BITS;
};
struct PrismSyncEvSlot_1 {
    unsigned char sync_ty : PRISM_SYNC_TYPE_BITS;
    unsigned char ev_ty : PRISM_EVENT_TYPE_BITS;
};

// Generated assembly is a bit shorter for these bit operations, versus casting to
// a struct w/ bitfields and assigning a bitfield. Bitfield method is better if we
// think alignment may be an issue for types larger than a byte.
//
// When repeatedly operating on the same set of bytes, Clang and GCC generate varying
// assembly w/ these macros depending on call depth and use of intermediate variables,
// so I'm not touching those optimizations.
// ----------------------------------------------------------------------------
#define SET_EV_TYPE_OR(ptr, EV_TY) ( *(unsigned char*)ptr |= ((unsigned char)EV_TY << 5) )
#define SET_EV_TYPE(ptr, EV_TY) ( *(unsigned char*)ptr = ((unsigned char)EV_TY << 5) )

// ----------------------------------------------------------------------------
#define SET_EV_CFG_TYPE(ptr, EV_TY) ( *(unsigned char*)ptr |= ((unsigned char)EV_TY << 2) )
#define SET_EV_CFG_CFG(ptr, CFG) ( *((unsigned char*)ptr + 1) = ((unsigned char)CFG) )
#define SET_EV_CFG_ALL(ptr, EV_TY, CFG) do {\
    SET_EV_TYPE(ptr, PRISM_EVENTTYPE_CFG);\
    SET_EV_CFG_TYPE(ptr, EV_TY);\
    SET_EV_CFG_CFG(ptr, CFG);\
} while (0)

// ----------------------------------------------------------------------------
#define SET_EV_MEM_TYPE(ptr, MEM_TY) ( *(unsigned char*)ptr |= ((unsigned char)MEM_TY << 3) )
#define SET_EV_MEM_SIZE(ptr, MEM_SZ) ( *(unsigned char*)ptr |= ((unsigned char)MEM_SZ) )
#define SET_EV_MEM_ADDR(ptr, MEM_ADDR) ( *((uintptr_t*)ptr) |= ((uintptr_t)MEM_ADDR << 8) )
#define SET_EV_MEM_ID_AFTER_ADDR(ptr, MEM_ID) ( *((unsigned char*)ptr+sizeof(uintptr_t)) = (unsigned char)(MEM_ID) )
#define SET_EV_MEM_ID(ptr, MEM_ID) ( *((unsigned char*)ptr+1) = (unsigned char)(MEM_ID) )

// ----------------------------------------------------------------------------
#define SET_EV_COMP_FMT(ptr, OP_FMT) ( *(unsigned char*)ptr |= ((unsigned char)OP_FMT << 3) )
#define SET_EV_COMP_SZ(ptr, OP_SZ) ( *(unsigned char*)ptr |= ((unsigned char)OP_SZ) )
#define SET_EV_COMP_TYPE(ptr, OP_TY) ( *((unsigned char*)ptr+1) |= ((unsigned char)OP_TY << 2) )
#define SET_EV_COMP_ARITY(ptr, OP_ARITY) ( *((unsigned char*)ptr+1) |= ((unsigned char)OP_ARITY) )
#define SET_EV_COMP_ID(ptr, SLOT, OP_ID) ( *((unsigned char*)ptr+2+SLOT) = ((unsigned char)OP_ID) )

// ----------------------------------------------------------------------------
#define SET_EV_CXT_TY(ptr, CXT_TY) ( *(unsigned char*)ptr |= ((unsigned char)CXT_TY) )
#define SET_EV_CXT_W_NAME(ptr, len, str, ty_w_name) do {\
    SET_EV_TYPE(ptr, PRISM_EVENTTYPE_CXT);\
    SET_EV_CXT_TY(ptr, ty_w_name);\
    ((unsigned char*)ptr)[1] = uint8_t{len} - 1;\
    memcpy((unsigned char*)ptr+2, str, len);\
} while (0)
#define SET_EV_CXT_W_ID(ptr, id, ty_w_id) do {\
    SET_EV_TYPE(ptr, PRISM_EVENTTYPE_CXT);\
    SET_EV_CXT_TY(ptr, ty_w_id);\
    *((uintptr_t*)ptr) |= ((uintptr_t)id << 8);\
} while (0)

#define SET_EV_OPENTER_NAME(ptr, len, str) SET_EV_CXT_W_NAME(ptr, len, str, PRISM_CXT_ML_OP_ENTER)
#define SET_EV_OPEXIT_NAME(ptr, len, str) SET_EV_CXT_W_NAME(ptr, len, str, PRISM_CXT_ML_OP_EXIT)
#define SET_EV_NETENTER_NAME(ptr, len, str) SET_EV_CXT_W_NAME(ptr, len, str, PRISM_CXT_ML_NET_ENTER)
#define SET_EV_NETEXIT_NAME(ptr, len, str) SET_EV_CXT_W_NAME(ptr, len, str, PRISM_CXT_ML_NET_EXIT)

// ----------------------------------------------------------------------------
#define SET_EV_SYNC_TY(ptr, SYNC_TY) ( *(unsigned char*)ptr |= ((unsigned char)SYNC_TY) )

// ----------------------------------------------------------------------------
#define SET_EV_END(ptr) SET_EV_TYPE(ptr, PRISM_EVENTTYPE_END)

#ifdef __cplusplus
} // end extern "C"
#endif

#endif //PRISM_PRIM_H

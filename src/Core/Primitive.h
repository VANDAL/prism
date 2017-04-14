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
 * XXX MDL20170414
 * These primitives are used for IPC between event generation frontends.
 * Due to the pure amount and variety of events transferred,
 * each struct is packed to save memory space.
 * Aligned structs do not have much performance benefit anyway, on x86,
 * due to the way events are serially written to and read in shared memory.
 * Otherwise, there is an overhead of up to 8x in memory space
 * (using unions and padding structs for alignment in arrays).
 */

#include <stdint.h>
#include "PrimitiveEnums.h"

#ifdef __cplusplus
#include <functional>
extern "C" {
#else
typedef struct SglMemEv SglMemEv;
typedef struct SglCompEv SglCompEv;
typedef struct SglCFEv SglCFEv;
typedef struct SglCxtEv SglCxtEv;
typedef struct SglSyncEv SglSyncEv;
#endif

typedef uintptr_t PtrVal;
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
    PtrVal    begin_addr;
    ByteCount size;
    MemType   type;
} __attribute__ ((__packed__));

struct SglCompEv
{
    CompCostType type;
    CompArity    arity;
    CompCostOp   op;
    uint8_t      size;
} __attribute__ ((__packed__));

struct SglCFEv
{
    /* unimplemented */

    CFType type;
} __attribute__ ((__packed__));

struct SglCxtEv
{
    CxtType type;
    union
    {
        PtrVal id;
        char*  name;
        struct
        {
            uint32_t idx;
            uint32_t len;
        };
    };
} __attribute__ ((__packed__));

struct SglSyncEv
{
    SyncType type;
    SyncID   data[2]; // optional data like mutex values
} __attribute__ ((__packed__));

#ifdef __cplusplus
} // end extern "C"

using GetNameBase = std::function<const char*(void)>;
namespace sigil2
{
/* XXX MDL20170414
 * Microtests show insignificant performance overhead for using const wrapper
 * structs that only access data from the underlying raw event data */

struct MemEvent
{
    MemEvent(const SglMemEv &ev) : ev(ev){}
    auto type() const -> MemType { return ev.type; }
    auto isLoad() const -> bool { return (ev.type == MemTypeEnum::SGLPRIM_MEM_LOAD); }
    auto isStore() const -> bool { return (ev.type == MemTypeEnum::SGLPRIM_MEM_STORE); }
    auto addr() const -> PtrVal { return ev.begin_addr; }
    auto bytes() const -> ByteCount { return ev.size; }
    const SglMemEv &ev;
};

struct CompEvent
{
    CompEvent(const SglCompEv &ev) : ev(ev) {}
    auto type() const -> CompCostType { return ev.type; }
    auto isIOP() const -> bool { return (ev.type == CompCostTypeEnum::SGLPRIM_COMP_IOP); }
    auto isFLOP() const -> bool { return (ev.type == CompCostTypeEnum::SGLPRIM_COMP_FLOP); }
    const SglCompEv &ev;
};

struct CxtEvent
{
    CxtEvent(const SglCxtEv &ev, const GetNameBase &nameBase)
        : ev(ev), nameBase(nameBase) {}
    auto type() const -> CxtType { return ev.type; }
    auto id() const -> PtrVal { return ev.id; }
    auto getName() const -> const char* { return ev.idx + nameBase(); }
    const SglCxtEv &ev;
  private:
    const GetNameBase &nameBase;
};

struct SyncEvent
{
    SyncEvent(const SglSyncEv &ev) : ev(ev) {}
    auto type() const -> SyncType { return ev.type; }
    auto data() const -> SyncID { return ev.data[0]; }
    auto dataExtra() const -> SyncID { return ev.data[1]; }
    const SglSyncEv &ev;
};
}; //end namespace sigil2
#endif

#endif //_SGLPRIM_H_

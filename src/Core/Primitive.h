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
 * They are EFFECTIVELY the raw data formats. Because a frontend (e.g.
 * instrumentation framework)
 * 1 may not support complex serialization libraries,
 * 2 and may have differing IPC capabilities,
 * this is as far as we define the events, and leave the rest of the
 * implementation up to the implementor to decide.
 *
 * These primitives are used for IPC between event generation frontends.
 * Due to the pure amount and variety of events transferred,
 * each struct is packed to save memory space.
 * Aligned structs do not have much performance benefit anyway, on x86,
 * due to the way events are serially written to and read in shared memory.
 * Otherwise, without packing, there is an overhead of up to 8x in memory
 * footprint (using unions and padding structs for alignment in arrays).
 */

#include <stdint.h>
#include "PrimitiveEnums.h"

#ifdef __cplusplus
#include <algorithm>
#include <cassert>
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

struct PrismMemEv
{
    PtrVal    begin_addr;
    ByteCount size;
    MemType   type;
} __attribute__ ((__packed__));

struct PrismCompEv
{
    CompCostType type;
    CompArity    arity;
    CompCostOp   op;
    uint8_t      size;
} __attribute__ ((__packed__));

struct PrismCFEv
{
    /* unimplemented */

    CFType type;
} __attribute__ ((__packed__));

struct PrismCxtEv
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

struct PrismSyncEv
{
    SyncType type;
    SyncID   data[2]; // optional data like mutex values

} __attribute__ ((__packed__));

#ifdef __cplusplus
} // end extern "C"

using GetNameBase = std::function<const char*(void)>;
namespace prism
{
/* XXX MDL20170414
 * Microtests show insignificant performance overhead for using const wrapper
 * structs that only access data from the underlying raw event data */

struct MemEvent
{
    MemEvent(const PrismMemEv &ev) : ev(ev){}
    auto type() const -> MemType { return ev.type; }
    auto isLoad() const -> bool { return (ev.type == MemTypeEnum::PRISM_MEM_LOAD); }
    auto isStore() const -> bool { return (ev.type == MemTypeEnum::PRISM_MEM_STORE); }
    auto addr() const -> PtrVal { return ev.begin_addr; }
    auto bytes() const -> ByteCount { return ev.size; }
    const PrismMemEv &ev;
};

struct CompEvent
{
    CompEvent(const PrismCompEv &ev) : ev(ev) {}
    auto type() const -> CompCostType { return ev.type; }
    auto isIOP() const -> bool { return (ev.type == CompCostTypeEnum::PRISM_COMP_IOP); }
    auto isFLOP() const -> bool { return (ev.type == CompCostTypeEnum::PRISM_COMP_FLOP); }
    const PrismCompEv &ev;
};

struct CxtEvent
{
    CxtEvent(const PrismCxtEv &ev, const GetNameBase &nameBase)
        : ev(ev), nameBase(nameBase) {}
    auto type() const -> CxtType { return ev.type; }
    auto id() const -> PtrVal { return ev.id; }
    auto getName() const -> const char* { return ev.idx + nameBase(); }
    const PrismCxtEv &ev;
  private:
    const GetNameBase &nameBase;
};

struct SyncEvent
{
    SyncEvent(const PrismSyncEv &ev) : ev(ev) {}
    auto type() const -> SyncType { return ev.type; }
    auto data() const -> SyncID { return ev.data[0]; }
    auto dataExtra() const -> SyncID { return ev.data[1]; }
    const PrismSyncEv &ev;
};


namespace capability
{
/* Each frontend has a set of 'event types' it can generate
 * and pass to backend analysis. To preserve flexibility in
 * the frontend, not all event types are necessarily required
 * to be supported.
 *
 * Because backends inherently require a subset of event types,
 * it is useful for a backend to be able to query whether an
 * event type is available for a given frontend.
 *
 * It is also useful for a frontend to know which event types
 * will be used, as the frontend can then *not* generate unused
 * events, improving efficiency.
 */

enum
{
    MEMORY = 0,
    MEMORY_LDST,
    MEMORY_ADDRESS,
    MEMORY_SIZE,

    COMPUTE,
    COMPUTE_INT_OR_FLOAT,
    COMPUTE_ARITY,
    COMPUTE_OP,
    COMPUTE_SIZE,

    CONTROL_FLOW,
    /* MDL20170720 unsupported currently */

    SYNC,
    SYNC_TYPE,
    SYNC_ARGS,
    /* MDL20170720 currently only two args can be captured */

    CONTEXT_INSTRUCTION,
    CONTEXT_BASIC_BLOCK,
    CONTEXT_FUNCTION,
    CONTEXT_THREAD,

    NUM_CAPABILITIES
};

enum availability
{
    nil = 0,
    disabled,
    enabled,
};

};

using capabilities = std::vector<capability::availability>;

inline auto initCaps()
{
    using namespace capability;
    return capabilities(NUM_CAPABILITIES, availability::nil);
}

namespace
{
using capability::availability;
inline auto resolveCaps_(availability fe, availability be)
{
    if (be == availability::enabled)
    {
        if (fe == availability::nil)
            throw std::invalid_argument("insufficient event capture capability");
        else
            return availability::enabled;
    }
    else
        return availability::disabled;
}
};

inline auto resolveCaps(const capabilities &feCaps, const capabilities &beReqs)
{
    using namespace capability;

    auto caps = initCaps();
    assert(feCaps.size() == NUM_CAPABILITIES &&
           beReqs.size() == NUM_CAPABILITIES &&
           caps.size() == NUM_CAPABILITIES);

    auto c = caps.begin();
    for (auto fc = feCaps.cbegin(), bc = beReqs.cbegin(), end = feCaps.cend(); fc != end; ++fc, ++bc)
        *c++ = resolveCaps_(*fc, *bc);
    return caps;
}

}; //end namespace prism
#endif

#endif //PRISM_PRIM_H

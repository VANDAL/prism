#ifndef PRISM_EVENTCAPABILITY_H
#define PRISM_EVENTCAPABILITY_H

#include <vector>

namespace prism::capability
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

enum options
{
    PRISMCAP_MEMORY = 0,
    PRISMCAP_MEMORY_LDST_TYPE,
    PRISMCAP_MEMORY_ACCESS_BYTES,
    PRISMCAP_MEMORY_ADDRESS,
    PRISMCAP_MEMORY_IDS,

    PRISMCAP_COMPUTE,
    PRISMCAP_COMPUTE_INT_OR_FLT,
    PRISMCAP_COMPUTE_WIDTH_BYTES,
    PRISMCAP_COMPUTE_OP_TYPE,
    PRISMCAP_COMPUTE_ARITY,
    PRISMCAP_COMPUTE_IDS,

    PRISMCAP_CONTROL_FLOW,
    /* MDL20170720 unsupported currently */

    PRISMCAP_SYNC,
    PRISMCAP_SYNC_TYPE,
    PRISMCAP_SYNC_ARGS,

    PRISMCAP_CONTEXT_INSTRUCTION,
    PRISMCAP_CONTEXT_BASIC_BLOCK,
    PRISMCAP_CONTEXT_FUNCTION,
    PRISMCAP_CONTEXT_THREAD,

    PRISMCAP_NUM_CAPABILITIES
};

enum availability
{
    PRISMCAP_UNAVAILABLE = 0,
    PRISMCAP_DISABLED,
    PRISMCAP_ENABLED,
    PRISMCAP_ALWAYS,
    // Some event properties cannot be disabled,
    // e.g. if a frontend always generates addresses.
};

using EvGenCaps = std::vector<capability::availability>;
auto initEvGenCaps() -> EvGenCaps;
auto resolveCaps(const EvGenCaps &caps_gen, const EvGenCaps &caps_req) -> EvGenCaps;

}  // end namespace prism::capability

#endif

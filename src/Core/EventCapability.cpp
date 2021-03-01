#include "EventCapability.hpp"
#include "PrismLog.hpp"
#include <cassert>
#include <stdexcept>

namespace prism::capability
{
auto initEvGenCaps() -> EvGenCaps
{
    return EvGenCaps(options::PRISMCAP_NUM_CAPABILITIES,
                     availability::PRISMCAP_UNAVAILABLE);
}

namespace
{
auto resolveCaps_(availability fe, availability be)
{
    if (be == availability::PRISMCAP_ENABLED)
    {
        if (fe == availability::PRISMCAP_UNAVAILABLE)
        {
            // TODO(someday) inform the user more elegantly than blurting out an exception
            throw std::invalid_argument("insufficient event capture capability");
        }
        else
        {
            return availability::PRISMCAP_ENABLED;
        }
    }
    else if (fe == availability::PRISMCAP_ALWAYS)
    {
        return availability::PRISMCAP_ENABLED;
    }
    else 
    {
        return availability::PRISMCAP_DISABLED;
    }
}
} // end namespace


auto resolveCaps(const EvGenCaps &caps_gen, const EvGenCaps &caps_req) -> EvGenCaps
{
    auto caps = initEvGenCaps();
    assert(caps_gen.size() == options::PRISMCAP_NUM_CAPABILITIES &&
           caps_req.size() == options::PRISMCAP_NUM_CAPABILITIES &&
           caps.size() == options::PRISMCAP_NUM_CAPABILITIES);

    auto rc = caps.begin();
    try {
        for (auto fc = caps_gen.cbegin(), bc = caps_req.cbegin(), end = caps_gen.cend(); fc != end; ++fc, ++bc, ++rc)
        {
            *rc = resolveCaps_(*fc, *bc);
        }
    } catch (const std::exception& e) {
        PrismLog::fatal("Caught exception: {}", e.what());
    }
    return caps;
}

} // end namespace prism::capability

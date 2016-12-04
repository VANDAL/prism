#ifndef SIGIL_FRONTEND_H
#define SIGIL_FRONTEND_H

#include <map>
#include <functional>
#include <string>
#include <vector>
#include <cstdint>

using ToolName = std::string;
using Args = std::vector<std::string>;

/* Frontend gets:
 * - the executable and its args
 * - args specifically for the frontend
 * - number of threads in the system */
using FrontendStarterArgs = std::tuple<Args, Args, unsigned>;
using FrontendStarter = std::function<void(FrontendStarterArgs)>;

/* The Sigil2 core will asynchronously request an event buffer from the frontend
 * to send events to the backend. The ownership of this buffer is acquired by
 * Sigil2 until it explicityly releases ownership back to the frontend.  That
 * is, for every acquire, there must be one and only one release.  Resource
 * acquisition must wait until the frontend has initialized by querying if the
 * frontend is ready. When the frontend runs out of events, a null pointer is
 * returned. */
struct EventBuffer;
using FrontendBufferAcquire = std::function<const EventBuffer*(unsigned idx)>;
using FrontendBufferRelease = std::function<void(unsigned idx)>;
using FrontendReady = std::function<bool(void)>;

struct Frontend
{
    FrontendStarter start;
    FrontendBufferAcquire acq;
    FrontendBufferRelease rel;
    FrontendReady ready;
    FrontendStarterArgs args;
};

class FrontendFactory
{
  public:
    FrontendFactory()  = default;
    ~FrontendFactory() = default;

    auto create(ToolName name, FrontendStarterArgs args) const -> Frontend;
    auto add(ToolName name, FrontendStarter start) -> void;
    auto add(ToolName name, FrontendBufferAcquire acq) -> void;
    auto add(ToolName name, FrontendBufferRelease rel) -> void;
    auto add(ToolName name, FrontendReady ready) -> void;
    auto exists(ToolName name) const -> bool;
    auto available() const -> std::vector<std::string>;

  private:
    std::map<ToolName, Frontend> registry;
};

#endif

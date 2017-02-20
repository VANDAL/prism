#ifndef SIGIL_FRONTEND_H
#define SIGIL_FRONTEND_H

#include <map>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

using ToolName = std::string;
using Args = std::vector<std::string>;


/* The Sigil2 core will asynchronously request an event buffer from the frontend
 * to send events to the backend. The ownership of this buffer is acquired by
 * Sigil2 until it explicityly releases ownership back to the frontend.  That
 * is, for every acquire, there must be one and only one release.  Resource
 * acquisition must wait until the frontend has initialized by querying if the
 * frontend is ready. When the frontend runs out of events, a null pointer is
 * returned. */
struct EventBuffer;
class FrontendIface
{
  public:
    FrontendIface() : uid(uidCount++) {}
    virtual ~FrontendIface() {}
    virtual auto acquireBuffer() -> EventBuffer* = 0;
    virtual auto releaseBuffer() -> void = 0;
  protected:
    const unsigned uid;
  private:
    static unsigned uidCount;
};

/* Frontend gets:
 * - the executable and its args
 * - args specifically for the frontend
 * - number of threads in the system */
using FrontendStarterArgs = std::tuple<Args, Args, unsigned>;
using FrontendPtr = std::shared_ptr<FrontendIface>;

/* The actual frontend must provide a 'starter' function that returns
 * a function to generate interfaces to the frontend as defined above.
 * In a multi-threaded frontend, each thread gets a separate interface
 * instance which shall provide access to a buffer; the buffer will
 * only be accessed by that instance/thread. */
using FrontendIfaceGenerator = std::function<FrontendPtr(void)>;
using FrontendStarter = std::function<FrontendIfaceGenerator(FrontendStarterArgs)>;
using FrontendStarterWrapper = std::function<FrontendIfaceGenerator(void)>;

class FrontendFactory
{
  public:
    FrontendFactory()  = default;
    ~FrontendFactory() = default;

    auto create(ToolName name, FrontendStarterArgs args) const -> FrontendStarterWrapper;
    auto add(ToolName name, FrontendStarter start)             -> void;
    auto exists(ToolName name)                           const -> bool;
    auto available()                                     const -> std::vector<std::string>;

  private:
    std::map<ToolName, FrontendStarter> registry;
};

#endif

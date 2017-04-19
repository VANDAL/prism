#ifndef SIGIL_FRONTEND_H
#define SIGIL_FRONTEND_H

#include "SigiLog.hpp"
#include "EventBuffer.h"
#include <map>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <cstdint>

using ToolName = std::string;
using Args = std::vector<std::string>;

using GetNameBase = std::function<const char*(void)>;
class FrontendIface
{
    /* The Sigil2 core asynchronously requests an event buffer
     * from the frontend to send events to the backend. */

  public:
    FrontendIface() : uid(uidCount++) {}
    virtual ~FrontendIface() {}

    virtual auto acquireBuffer() -> EventBufferPtr = 0;
    virtual auto releaseBuffer(EventBufferPtr) -> void = 0;
    /* The ownership of this buffer is acquired by
     * Sigil2 until it explicitly releases ownership back to the frontend.
     * That is, for every acquire, there shall be one and only one release.
     * When the frontend runs out of events, a null pointer is returned. */

    GetNameBase nameBase;
    /* If a frontend supports names for context events, e.g. function names,
     * it must implement this function to return the memory arena where
     * name strings are stored. XXX MDL20170412 See DbiFrontend.hpp */

  protected:
    const unsigned uid;
  private:
    static std::atomic<unsigned> uidCount;
};


using FrontendStarterArgs = std::tuple<Args, Args, unsigned>;
/* Frontend gets:
 * - the executable and its args
 * - args specifically for the frontend
 * - number of threads in the system */

using FrontendPtr = std::unique_ptr<FrontendIface>;
using FrontendIfaceGenerator = std::function<FrontendPtr(void)>;
using FrontendStarter = std::function<FrontendIfaceGenerator(FrontendStarterArgs)>;
using FrontendStarterWrapper = std::function<FrontendIfaceGenerator(void)>;
/* The actual frontend must provide a 'starter' function that returns
 * a function to generate interfaces to the frontend as defined above.
 * In a multi-threaded frontend, each thread gets a separate interface
 * instance which shall provide access to a buffer; the buffer will
 * only be accessed by that instance/thread. */

class FrontendFactory
{
  public:
    FrontendFactory()  = default;
    ~FrontendFactory() = default;

    auto create(ToolName name, FrontendStarterArgs args) const -> FrontendStarterWrapper;
    auto add(ToolName name, FrontendStarter start) -> void;
    auto exists(ToolName name) const -> bool;
    auto available() const -> std::vector<std::string>;

  private:
    std::map<ToolName, FrontendStarter> registry;
};

#endif

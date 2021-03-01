#ifndef PRISM_FRONTEND_H
#define PRISM_FRONTEND_H

#include "EventBuffer.h"
#include "EventIface.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

using ToolName = std::string;
using Args = std::vector<std::string>;

class FrontendIface
{
    /* The Prism core asynchronously requests an event buffer
     * from the frontend to send events to the backend. */

  public:
    FrontendIface() : uid(uidCount++) {}
    FrontendIface(const FrontendIface&) = delete;
    FrontendIface& operator=(const FrontendIface&) = delete;
    virtual ~FrontendIface() {}

    virtual auto acquireBuffer() -> EventBufferPtr = 0;
    virtual auto releaseBuffer(EventBufferPtr) -> void = 0;
    /* The ownership of this buffer is acquired by
     * Prism until it explicitly releases ownership back to the frontend.
     * That is, for every acquire, there shall be one and only one release.
     * When the frontend runs out of events, a null pointer is returned. */

  protected:
    const unsigned uid;
  private:
    static std::atomic<unsigned> uidCount;
};


using FrontendPtr = std::unique_ptr<FrontendIface>;
using FrontendIfaceGenerator = std::function<FrontendPtr()>;
using FrontendStarter = std::function<FrontendIfaceGenerator(Args, Args, unsigned,
                                                             const prism::capability::EvGenCaps&)>;
using FrontendStarterWrapper = std::pair<prism::EventStreamParserConfig, std::function<FrontendIfaceGenerator()>>;
/* The actual frontend must provide a 'starter' function that returns
 * a function to generate interfaces to the frontend as defined above.
 * In a multi-threaded frontend, each thread gets a separate interface instance
 * which shall provide access to a buffer; the buffer will only be accessed
 * by that instance/thread.
 *
 * Frontend start function gets:
 * - the executable and its args
 * - args specifically for the frontend
 * - number of threads in the system
 * - requested capabilities from backend */


struct Frontend
{
    FrontendStarter starter;
    prism::capability::EvGenCaps caps;
};


class FrontendFactory
{
  public:
    FrontendFactory()  = default;
    ~FrontendFactory() = default;

    auto create(
        ToolName name,
        Args exec,
        Args fe,
        unsigned threads,
        const prism::capability::EvGenCaps &beReqs) const -> FrontendStarterWrapper;

    auto add(ToolName name, Frontend fe) -> void;
    auto exists(ToolName name) const -> bool;
    auto available() const -> std::vector<std::string>;

  private:
    std::map<ToolName, Frontend> registry;
};

#endif

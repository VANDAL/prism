#ifndef PRISM_FRONTEND_H
#define PRISM_FRONTEND_H

#include "PrismLog.hpp"
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
    /* The Prism core asynchronously requests an event buffer
     * from the frontend to send events to the backend. */

  public:
    FrontendIface() : uid(uidCount++) {}
    virtual ~FrontendIface() {}

    virtual auto acquireBuffer() -> EventBufferPtr = 0;
    virtual auto releaseBuffer(EventBufferPtr) -> void = 0;
    /* The ownership of this buffer is acquired by
     * Prism until it explicitly releases ownership back to the frontend.
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


using FrontendPtr = std::unique_ptr<FrontendIface>;
using FrontendIfaceGenerator = std::function<FrontendPtr(void)>;
using FrontendStarter = std::function<FrontendIfaceGenerator(Args, Args, unsigned,
                                                             const prism::capabilities&)>;
using FrontendStarterWrapper = std::function<FrontendIfaceGenerator()>;
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
    prism::capabilities caps;
};


class FrontendFactory
{
  public:
    FrontendFactory()  = default;
    ~FrontendFactory() = default;

    auto create(ToolName name, Args exec, Args fe, unsigned threads,
                const prism::capabilities &beReqs) const -> FrontendStarterWrapper;
    auto add(ToolName name, Frontend fe) -> void;
    auto exists(ToolName name) const -> bool;
    auto available() const -> std::vector<std::string>;

  private:
    std::map<ToolName, Frontend> registry;
};

#endif

#include "Config.hpp"
#include "EventBuffer.h"

#include "Frontends/AvailableFrontends.hpp"

#include "Backends/SynchroTraceGen/EventHandlers.hpp"
#include "Backends/SimpleCount/Handler.hpp"
#include "Backends/SigilClassic/Handler.hpp"

using namespace PrismLog;
using namespace prism;

namespace
{
auto startPrism(const Config& config) -> int;
};

int main(int argc, char* argv[])
{
    auto config = Config()
        .registerFrontend("valgrind",
                          {startGengrind,
                          gengrindCapabilities()})
        .registerFrontend("dynamorio",
                          {startDrSigil,
                          drSigilCapabilities()})
        .registerFrontend("perf",
                          {startPerfPT,
                          perfPTCapabilities()})
        .registerBackend("stgen",
                         []{return std::make_unique<::STGen::EventHandlers>();},
                         ::STGen::onParse,
                         ::STGen::onExit,
                         ::STGen::requirements())
        .registerBackend("simplecount",
                         []{return std::make_unique<::SimpleCount::Handler>();},
                         {},
                         ::SimpleCount::cleanup,
                         ::SimpleCount::requirements())
        .registerBackend("sigilclassic",
                         []{return std::make_unique<::SigilClassic::Handler>();},
                         {},
                         {},
                         initCaps())
        .registerBackend("null",
                         []{return std::make_unique<::BackendIface>();},
                         {},
                         {},
                         initCaps())
        .parseCommandLine(argc, argv);

    return startPrism(config);
}


namespace
{

auto flushToBackend(BackendIface &be,
                    const EventBuffer &buf,
                    const GetNameBase &nameBase) -> void
{
    for (decltype(buf.used) i = 0; i < buf.used; ++i)
    {
        const PrismEvVariant &ev = buf.events[i];

        switch (ev.tag)
        {
        case EvTagEnum::PRISM_MEM_TAG:
            be.onMemEv({ev.mem});
            break;
        case EvTagEnum::PRISM_COMP_TAG:
            be.onCompEv({ev.comp});
            break;
        case EvTagEnum::PRISM_SYNC_TAG:
            be.onSyncEv({ev.sync});
            break;
        case EvTagEnum::PRISM_CXT_TAG:
            be.onCxtEv({ev.cxt, nameBase});
            break;
        case EvTagEnum::PRISM_CF_TAG:
            be.onCFEv(ev.cf);
            break;
        default:
            fatal("Received unhandled event in " __FILE__);
        }
    }
}


auto consumeEvents(BackendIfaceGenerator createBEIface,
                   FrontendIfaceGenerator createFEIface) -> void
{
    BackendPtr backendIface  = createBEIface();
    FrontendPtr frontendIface = createFEIface();
    /* per-thread frontend/backend interfaces
     * each backend interface needs a frontend interface to communicate with */

    EventBufferPtr buf = frontendIface->acquireBuffer();

    while (buf != nullptr) // consume events until there's nothing left
    {
        flushToBackend(*backendIface, *buf,
                       frontendIface->nameBase);

        /* acquire a new buffer */
        frontendIface->releaseBuffer(std::move(buf));
        buf = frontendIface->acquireBuffer();
    }
}


auto startPrism(const Config& config) -> int
{
    using std::chrono::high_resolution_clock;

    auto threads       = config.threads();
    auto backend       = config.backend();
    auto startFrontend = config.startFrontend();
    auto timed         = config.timed();

    if (threads < 1)
        fatal("Invalid number of backend threads");

    if (backend.parser)
        backend.parser(backend.args);
    else if (backend.args.size() > 0)
        fatal("Backend arguments provided, but Backend has no parser");

    info("executable : " + config.executablePrintable());
    info("frontend   : " + (config.frontendPrintable().empty() ? "default" : config.frontendPrintable()));
    info("backend    : " + config.backendPrintable());
    info("threads    : " + config.threadsPrintable());
    info("timed      : " + (timed ? std::string("on") : std::string("off")));

    /* start frontend only once and get its interface */
    auto frontendIfaceGenerator = startFrontend();
    std::vector<std::thread> eventStreams;
    for(auto i = 0; i < threads; ++i)
        eventStreams.emplace_back(std::thread(consumeEvents,
                                              backend.generator,
                                              frontendIfaceGenerator));

    high_resolution_clock::time_point start, end;
    if (timed == true)
        start = high_resolution_clock::now();

    /* wait for event handling to finish and then clean up */
    for(auto i = 0; i < threads; ++i)
        eventStreams[i].join();
    if (backend.finish)
        backend.finish();

    if (timed == true)
    {
        end = high_resolution_clock::now();
        auto ms = std::chrono::duration<double>(end - start);
        info("Prism duration: " + std::to_string(ms.count()) + "s");
    }

    return EXIT_SUCCESS;
}

}; //end namespace

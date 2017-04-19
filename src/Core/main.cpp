#include "Config.hpp"
#include "EventBuffer.h"

#include "Frontends/AvailableFrontends.hpp"

#include "Backends/SynchroTraceGen/EventHandlers.hpp"
#include "Backends/SimpleCount/Handler.hpp"
#include "Backends/SigilClassic/Handler.hpp"

#ifdef PRETTY_PRINT_TITLE
#include <iostream>
#endif

using namespace SigiLog;
using namespace sigil2;

namespace
{
auto startSigil2(const Config& config) -> int;
};

int main(int argc, char* argv[])
{
#ifdef PRETTY_PRINT_TITLE
    std::string title =
        "    ______    _           _  __   _____   \n"
        "  .' ____ \\  (_)         (_)[  | / ___ `. \n"
        "  | (___ \\_| __   .--./) __  | ||_/___) | \n"
        "   _.____`. [  | / /'`\\;[  | | | .'____.' \n"
        "  | \\____) | | | \\ \\._// | | | |/ /_____  \n"
        "   \\______.'[___].',__` [___|___]_______| \n"
        "                 ( ( __))                 \n"
        "                                          \n";
    std::cerr << title;
#endif

    auto config = Config()
        .registerFrontend("valgrind",
                          startSigrind)
        .registerFrontend("dynamorio",
                          startDrSigil)
        .registerBackend("stgen",
                         {[]{return std::make_unique<::STGen::EventHandlers>();},
                          ::STGen::onParse,
                          ::STGen::onExit,
                          {},})
        .registerBackend("simplecount",
                         {[]{return std::make_unique<::SimpleCount::Handler>();},
                          {},
                          ::SimpleCount::cleanup,
                          {},})
        .registerBackend("sigilclassic",
                         {[]{return std::make_unique<::SigilClassic::Handler>();},
                          {},
                          {},
                          {},})
        .registerBackend("null",
                         {[]{return std::make_unique<::BackendIface>();},
                          {},
                          {},
                          {},})
        .parseCommandLine(argc, argv);

    return startSigil2(config);
}


namespace
{

auto flushToBackend(BackendIface &be,
                    const EventBuffer &buf,
                    const GetNameBase &nameBase) -> void
{
    for (decltype(buf.used) i = 0; i < buf.used; ++i)
    {
        const SglEvVariant &ev = buf.events[i];

        switch (ev.tag)
        {
        case EvTagEnum::SGL_MEM_TAG:
            be.onMemEv({ev.mem});
            break;
        case EvTagEnum::SGL_COMP_TAG:
            be.onCompEv({ev.comp});
            break;
        case EvTagEnum::SGL_SYNC_TAG:
            be.onSyncEv({ev.sync});
            break;
        case EvTagEnum::SGL_CXT_TAG:
            be.onCxtEv({ev.cxt, nameBase});
            break;
        case EvTagEnum::SGL_CF_TAG:
            be.onCFEv(ev.cf);
            break;
        default:
            fatal(std::string("Received unhandled event in ").append(__FILE__));
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


auto startSigil2(const Config& config) -> int
{
    auto threads       = config.threads();
    auto backend       = config.backend();
    auto startFrontend = config.startFrontend();

    if (threads < 1)
        fatal("Invalid number of backend threads");

    if (backend.parser)
        backend.parser(backend.args);
    else if (backend.args.size() > 0)
        fatal("Backend arguments provided, but Backend has no parser");

    /* start frontend only once and get its interface */
    auto frontendIfaceGenerator = startFrontend();
    std::vector<std::thread> eventStreams;
    for(auto i = 0; i < threads; ++i)
        eventStreams.emplace_back(std::thread(consumeEvents,
                                              backend.generator,
                                              frontendIfaceGenerator));

    /* wait for event handling to finish and then clean up */
    for(auto i = 0; i < threads; ++i)
        eventStreams[i].join();
    if (backend.finish)
        backend.finish();

    return EXIT_SUCCESS;
}

}; //end namespace

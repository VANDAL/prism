#include "Sigil2Config.hpp"
#include "EventBuffer.h"

#include "Frontends/AvailableFrontends.hpp"
#include "Backends/SynchroTraceGen/EventHandlers.hpp"
#include "Backends/SimpleCount/Handler.hpp"

#include <iostream>

using namespace SigiLog;

auto startSigil2(const Sigil2Config& config) -> int;

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

    auto config = Sigil2Config()
        .registerFrontend("valgrind",
                          startSigrind)
        .registerFrontend("dynamorio",
                          startDrSigil)
        .registerBackend("stgen",
                         {[]() {return std::make_shared<::STGen::EventHandlers>();},
                          ::STGen::onParse,
                          ::STGen::onExit,
                          {},}
                        )
        .registerBackend("simplecount",
                         {[]() {return std::make_shared<::SimpleCount::Handler>();},
                          {},
                          ::SimpleCount::cleanup,
                          {},}
                        )
        .parseCommandLine(argc, argv);


    return startSigil2(config);
}


namespace
{
auto inline flushToBackend(std::shared_ptr<BackendIface>& be, EventBuffer* buf) -> void
{
    for (decltype(buf->events_used) i = 0; i < buf->events_used; ++i)
    {
        BufferedSglEv &ev = buf->events[i];

        switch (ev.tag)
        {
        case EvTagEnum::SGL_MEM_TAG:
            be->onMemEv(ev.mem);
            break;

        case EvTagEnum::SGL_COMP_TAG:
            be->onCompEv(ev.comp);
            break;

        case EvTagEnum::SGL_SYNC_TAG:
            be->onSyncEv(ev.sync);
            break;

        case EvTagEnum::SGL_CXT_TAG:
            /* pool index set by frontend, so convert addressto this memory space */
            if (ev.cxt.type == SGLPRIM_CXT_FUNC_ENTER || ev.cxt.type == SGLPRIM_CXT_FUNC_EXIT)
                ev.cxt.name = buf->pool + ev.cxt.idx;
            be->onCxtEv(ev.cxt);
            break;

        case EvTagEnum::SGL_CF_TAG:
            be->onCFEv(ev.cf);
            break;

        default:
            fatal(std::string("Received unhandled event in ").append(__FILE__));
        }
    }
}

auto consumeEvents(BackendGenerator createBEIface, FrontendIfaceGenerator createFEIface) -> void
{
    /* per-thread frontend/backend interfaces */
    auto frontendIface = createFEIface();
    auto backendIface  = createBEIface();

    EventBuffer* buf = frontendIface->acquireBuffer();
    while (buf != nullptr) // consume events until there's nothing left
    {
        flushToBackend(backendIface, buf);

        /* acquire a new buffer */
        frontendIface->releaseBuffer();
        buf = frontendIface->acquireBuffer();
    }
}
};

auto startSigil2(const Sigil2Config& config) -> int
{
    auto threads       = config.threads();
    auto backend       = config.backend();
    auto startFrontend = config.startFrontend();

    /* let the backend parse its args */
    if (backend.args.size() > 0)
    {
        if (backend.parser)
            backend.parser(backend.args);
        else
        {
            error("Backend arguments provided, but Backend has no parser");
            return EXIT_FAILURE;
        }
    }

    /* start async backend event processing */
    if (threads < 1)
    {
        error("Invalid number of backend threads");
        return EXIT_FAILURE;
    }

    /* each backend interface needs a frontend interface to communicate with */
    auto frontendIfaceGenerator = startFrontend();
    std::vector<std::thread> backends;
    for(auto i = 0; i < threads; ++i)
        backends.emplace_back(std::thread(consumeEvents,
                                          backend.generator,
                                          frontendIfaceGenerator));

    /* wait for backends to finish and then clean up */
    for(auto i = 0; i < threads; ++i)
        backends[i].join();

    if (backend.finish)
        backend.finish();

    return EXIT_SUCCESS;
}

#include "Sigil2.hpp"
#include "EventBuffer.h"
#include "SigiLog.hpp"
#include <thread>

using namespace SigiLog;

auto startSigil2(const Sigil2Config& config) -> int
{
    auto threads  = config.threads();
    auto backend  = config.backend();
    auto frontend = config.frontend();

    /* let the backend parse its args */
    if (backend.args.size() > 1)
    {
        if (backend.parser)
        {
            backend.parser(backend.args);
        }
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

    std::vector<std::thread> backends;
    for(auto i = 0; i < threads; ++i)
        backends.emplace_back(std::thread(consumeEvents, backend.generator, i,
                                          frontend.acq, frontend.rel, frontend.ready));

    /* start frontend; return upon completion */
    frontend.start(frontend.args);

    /* wait for backends to finish and then clean up */
    for(auto i = 0; i < threads; ++i)
        backends[i].join();

    if (backend.finish)
        backend.finish();

    return EXIT_SUCCESS;
}


#include <chrono>
#include <iostream>
auto consumeEvents(BackendGenerator be, int idx,
                   FrontendBufferAcquire acq,
                   FrontendBufferRelease rel,
                   FrontendReady ready) -> void
{
    /* wait for frontend to become available */
    while(ready() == false);

    /* new backend per thread */
    auto backend = be();

    /* consume events until there's nothing left */
    const EventBuffer* buf = acq(idx);
    while (buf != nullptr)
    {
        for (decltype(buf->events_used) i = 0; i < buf->events_used; ++i)
        {
            const BufferedSglEv &ev = buf->events[i];

            switch (ev.tag)
            {
            case EvTagEnum::SGL_MEM_TAG:
                backend->onMemEv(ev.mem);
                break;

            case EvTagEnum::SGL_COMP_TAG:
                backend->onCompEv(ev.comp);
                break;

            case EvTagEnum::SGL_SYNC_TAG:
                backend->onSyncEv(ev.sync);
                break;

            case EvTagEnum::SGL_CXT_TAG:
                backend->onCxtEv(ev.cxt);
                break;

            case EvTagEnum::SGL_CF_TAG:
                backend->onCFEv(ev.cf);
                break;

            default:
                fatal(std::string("Received unhandled event in ").append(__FILE__));
            }
        }

        /* acquire a new buffer */
        rel(idx);
        buf = acq(idx);
    }
}

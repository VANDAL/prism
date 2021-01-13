#include "CoreEventLoop.hpp"
#include "EventBuffer.h"
#include "Frontends/AvailableFrontends.hpp"

#include  <chrono>
#include  <thread>
#include  <iostream>

using namespace PrismLog;

namespace prism {

auto flushToBackend(BackendIface &be,
                    EventStreamParserConfig &cfg,
                    const unsigned char* buf_base) -> size_t
{
    size_t num_events = 0;
    const unsigned char* buf = buf_base;

    for (unsigned char ev_ty = (*buf >> 5); ev_ty != EventTypeEnum::PRISM_EVENTTYPE_END; num_events++)
    {
        switch(ev_ty) {
        case EventTypeEnum::PRISM_EVENTTYPE_MEM:
            be.onMemEv({buf});
            buf += cfg.cfg_bytes_mem_ev;
            break;
        case EventTypeEnum::PRISM_EVENTTYPE_COMP:
            be.onCompEv({buf});
            buf += cfg.comp_id_bytes(reinterpret_cast<const std::byte*>(buf));
            break;
        case EventTypeEnum::PRISM_EVENTTYPE_SYNC:
            be.onSyncEv({buf});
            if (const auto ty = SyncEvent::type(buf); ty == SyncTypeEnum::PRISM_SYNC_CONDWAIT) {
                buf += 2 * sizeof(uintptr_t);
            } else {
                buf += 1 * sizeof(uintptr_t);
            }
            buf += cfg.cfg_bytes_sync_ev;
            break;
        case EventTypeEnum::PRISM_EVENTTYPE_CXT:
            be.onCxtEv({buf});
            switch (CxtEvent::type(buf)) {
            case CxtTypeEnum::PRISM_CXT_INSTR:
            case CxtTypeEnum::PRISM_CXT_BB:
            case CxtTypeEnum::PRISM_CXT_THREAD:
                // Each valid context event has an associated 7-byte ID/address.
                // This follows from memory events which can have a 56-bit address.
                buf += sizeof(uintptr_t) - 1;
                break;
            case CxtTypeEnum::PRISM_CXT_FUNC_ENTER:
            case CxtTypeEnum::PRISM_CXT_FUNC_EXIT:
                // Each string starts with a 1-byte length, and is at least 1-char after (1-256).
                // So incrememnt for the length, and offset the length value (0->1, 255->256).
                buf += 1 + (*(buf+1) + 1);
                break;
            default:
                fatal("Received unhandled context type after {} events, at buffer offset {}, in " __FILE__" : {}", num_events, buf-buf_base, CxtEvent::type(buf));
                break;
            }
            buf += cfg.cfg_bytes_cxt_ev;
            break;
        case EventTypeEnum::PRISM_EVENTTYPE_CFG:
            cfg.update_evt_cfgs(buf);
            buf += cfg.cfg_bytes_cfg_ev;
            break;
        default:
            fatal("Received unhandled event type in " __FILE__);
            break;
        }

        ev_ty = *buf >> 5;
    }

    num_events++;
    return num_events;
}


auto consumeEvents(BackendIfaceGenerator createBEIface,
                   FrontendIfaceGenerator createFEIface,
                   EventStreamParserConfig evStreamParserCfg) -> void
{
    BackendPtr backendIface  = createBEIface();
    FrontendPtr frontendIface = createFEIface();
    /* per-thread frontend/backend interfaces
     * each backend interface needs a frontend interface to communicate with */

    EventBufferPtr buf = frontendIface->acquireBuffer();

    while (buf != nullptr) // consume events until there's nothing left
    {
        flushToBackend(*backendIface, evStreamParserCfg, buf->events);

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
    auto [evStreamParserCfg, startFrontend] = config.startFrontend();
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
                                              frontendIfaceGenerator,
                                              evStreamParserCfg));

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

} // end namespace prism

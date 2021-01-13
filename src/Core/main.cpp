#include "CoreEventLoop.hpp"

#include "Backends/SynchroTraceGen/EventHandlers.hpp"
#include "Backends/SimpleCount/Handler.hpp"
#include "Frontends/AvailableFrontends.hpp"


using namespace PrismLog;
using namespace prism;

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
        .registerBackend("null",
                         []{return std::make_unique<::BackendIface>();},
                         {},
                         {},
                         capability::initEvGenCaps())
        .parseCommandLine(argc, argv);

    return startPrism(config);
}

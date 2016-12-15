#include "Sigil2.hpp"
#include <iostream>

#include "Frontends/Sigrind/Sigrind.hpp"
#include "Backends/SynchroTraceGen/EventHandlers.hpp"
#include "Backends/SimpleCount/Handler.hpp"

auto prettyPrintSigil2() -> void
{
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
}


int main(int argc, char* argv[])
{
#ifdef PRETTY_PRINT_TITLE
    prettyPrintSigil2();
#endif

    auto config = Sigil2Config()
        .registerFrontend("valgrind",
                          {sgl::startSigrind,
                           sgl::acqBufferFromSigrind,
                           sgl::relBufferFromSigrind,
                           sgl::sigrindReady,
                          })
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

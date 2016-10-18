#include "Plugins.hpp"

/* Static plugins are registered to Sigil here
 * TODO dynamic plugins */

#include "SimpleCount/Handler.hpp"
SIGIL_REGISTER(simplecount)
{
    BACKEND(SimpleCount::Handler)
    EXIT(SimpleCount::cleanup)
}

#include "SigilClassic/Handler.hpp"
SIGIL_REGISTER(sigilclassic)
{
    BACKEND(SigilClassic::Handler)
}

#include "SynchroTraceGen/EventHandlers.hpp"
SIGIL_REGISTER(stgen)
{
    BACKEND(STGen::EventHandlers)
    PARSER(STGen::onParse)
    EXIT(STGen::onExit)
}

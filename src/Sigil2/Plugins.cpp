#include "Plugins.hpp"

/* Static plugins are registered to Sigil here
 * TODO dynamic plugins */

/*
#include "MySigilBackend.h"
SIGIL_REGISTER(MyBackEnd)
*/

#include "SynchroTraceGen/EventHandlers.hpp"
SIGIL_REGISTER(stgen)
{
    BACKEND(STGen::EventHandlers)
    PARSER(STGen::onParse)
    EXIT(STGen::onExit);
}

//#include "Dummy/Dummy.hpp"
//SIGIL_REGISTER(dummy)
//{
//	/* example with regular functions */
//	/* argument parser is optional */
//}

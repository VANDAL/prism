#include "Plugins.hpp"

/* Static plugins are registered to Sigil here
 * TODO dynamic plugins */

/*
#include "MySigilBackend.h"
SIGIL_REGISTER(MyBackEnd)
{
	
}
*/

#include "SynchroTraceGen/EventHandlers.hpp"
SIGIL_REGISTER(STGen)
{
	/* example using instance backend */
	static STGen::EventHandlers handler;
	EVENT_HANDLER([](SglMemEv ev){handler.onMemEv(ev);});
	EVENT_HANDLER([](SglCompEv ev){handler.onCompEv(ev);});
	EVENT_HANDLER([](SglSyncEv ev){handler.onSyncEv(ev);});
	FINISH([](){handler.cleanup();});
	PARSER([](std::vector<std::string> args){handler.parseArgs(args);});
}

#include "Dummy/Dummy.hpp"
SIGIL_REGISTER(dummy)
{
	/* example with regular functions */
	EVENT_HANDLER(dummy::countMems);
	EVENT_HANDLER(dummy::countComps);
	EVENT_HANDLER(dummy::countSyncs);
	FINISH(dummy::cleanup);
	/* argument parser is optional */
}

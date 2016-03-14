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
	/* calling a member function from std::bind
	 * was causing too many allocations during
	 * event analysis */
	EVENT_HANDLER(STGen::onMemEv);
	EVENT_HANDLER(STGen::onCompEv);
	EVENT_HANDLER(STGen::onSyncEv);
	FINISH(STGen::cleanup);
	PARSER(STGen::parseArgs);
}

#include "Dummy/Dummy.hpp"
SIGIL_REGISTER(dummy)
{
	EVENT_HANDLER(dummy::countMems);
	EVENT_HANDLER(dummy::countComps);
	EVENT_HANDLER(dummy::countSyncs);
	FINISH(dummy::cleanup);
}

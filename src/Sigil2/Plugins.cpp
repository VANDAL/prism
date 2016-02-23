#include "Plugins.hpp"

/* Static plugins are registered to Sigil here
 * TODO dynamic plugins
 */

/*
#include "MySigilBackend.h"
SIGIL_REGISTER(MyBackEnd)
{
	
}
*/

#include "SynchroTraceGen/EventHandlers.hpp"
SIGIL_REGISTER(STGen)
{
	/* calling a member function from std::bind was causing too many mallocs/frees */
	EVENT_HANDLER(STGen::onMemEv);
	EVENT_HANDLER(STGen::onCompEv);
	EVENT_HANDLER(STGen::onSyncEv);
	FINISH(STGen::cleanup);
}

#include "Dummy/Dummy.hpp"
SIGIL_REGISTER(dummy)
{
	EVENT_HANDLER(countMems);
	EVENT_HANDLER(countComps);
	EVENT_HANDLER(countSyncs);
	FINISH(cleanup);
}

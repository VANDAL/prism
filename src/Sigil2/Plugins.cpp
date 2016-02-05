#include "Plugins.hpp"
#include "EventManager.hpp"

namespace
{
std::function<sgl::EventManager&(void)> sigil = sgl::EventManager::instance; 
using std::placeholders::_1;
};

/* Static plugins are registered to Sigil 
 * via a void function that takes no arguments.
 *
 * Users can add functionality by adding their
 * register functions in this header and its
 * corresponding source file.
 *
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
	sigil().addObserver(STGen::onSyncEv);
	sigil().addObserver(STGen::onCompEv);
	sigil().addObserver(STGen::onMemEv);
	sigil().addCleanup(STGen::cleanup);
}

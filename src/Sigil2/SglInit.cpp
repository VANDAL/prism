#include "SglInit.hpp"
#include "Sigil.hpp"

// User back ends
#include "SynchroTraceGen/EventHandlers.hpp"

void setSynchroTrace()
{
	/* TODO 
	 * dangerous, intentionally not deleting to allow Sigil to use
	 * this instance after this function returns. When to delete? */
	STGen::EventHandlers* handler = new STGen::EventHandlers();
	Sigil::getInstance().mgr.addObserver(std::bind(&STGen::EventHandlers::onCompEv, handler, std::placeholders::_1));
	Sigil::getInstance().mgr.addObserver(std::bind(&STGen::EventHandlers::onMemEv, handler, std::placeholders::_1));
	Sigil::getInstance().mgr.addObserver(std::bind(&STGen::EventHandlers::onSyncEv, handler, std::placeholders::_1));
	Sigil::getInstance().mgr.addObserver(std::bind(&STGen::EventHandlers::onCxtEv, handler, std::placeholders::_1));
}

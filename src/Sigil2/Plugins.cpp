#include "Plugins.hpp"
#include "EventManager.hpp"
static std::function<sgl::EventManager&(void)> sigil = sgl::EventManager::instance;

#include "SynchroTraceGen/EventHandlers.hpp"
void registerSynchroTrace()
{
	static STGen::EventHandlers handler;
	sigil().addObserver(std::bind(&STGen::EventHandlers::onCompEv, handler, std::placeholders::_1));
	sigil().addObserver(std::bind(&STGen::EventHandlers::onMemEv, handler, std::placeholders::_1));
	sigil().addObserver(std::bind(&STGen::EventHandlers::onSyncEv, handler, std::placeholders::_1));
	sigil().addObserver(std::bind(&STGen::EventHandlers::onCxtEv, handler, std::placeholders::_1));
}

/*
#include "MySigilBackend.h"
void registerMySigilBackend()
{
	
}
 */

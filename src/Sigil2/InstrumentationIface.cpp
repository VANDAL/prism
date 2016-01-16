#include "InstrumentationIface.h"
#include "EventManager.hpp"

using sgl::EventManager;

void SGLnotifyMem(SglMemEv ev)
{
	EventManager::instance().addEvent(ev);
}

void SGLnotifyComp(SglCompEv ev)
{
	EventManager::instance().addEvent(ev);
}

void SGLnotifyCxt(SglCxtEv ev)
{
	EventManager::instance().addEvent(ev);
}

void SGLnotifySync(SglSyncEv ev)
{
	EventManager::instance().addEvent(ev);
}

void SGLnotifyCF(SglCFEv ev)
{
}

void SGLnotifyFinish()
{
	EventManager::instance().finish();
}

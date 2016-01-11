#include "FrontIface.h"
#include "Sigil.hpp"

void SGLnotifyMem(SglMemEv ev)
{
	Sigil::getInstance().mgr.addEvent(ev);
}

void SGLnotifyComp(SglCompEv ev)
{
	Sigil::getInstance().mgr.addEvent(ev);
}

void SGLnotifyCxt(SglCxtEv ev)
{
	Sigil::getInstance().mgr.addEvent(ev);
}

void SGLnotifySync(SglSyncEv ev)
{
	Sigil::getInstance().mgr.addEvent(ev);
}

void SGLnotifyCF(SglCFEv ev)
{
}

void SGLnotifyFinish()
{
	Sigil::getInstance().mgr.flushEvents();
}

#include "InstrumentationIface.h"
#include "Sigil.hpp"

using sgl::EventManager;

void SGLnotifyMem(SglMemEv ev)
{
	Sigil::instance().addEvent(ev);
}

void SGLnotifyComp(SglCompEv ev)
{
	Sigil::instance().addEvent(ev);
}

void SGLnotifyCxt(SglCxtEv ev)
{
	Sigil::instance().addEvent(ev);
}

void SGLnotifySync(SglSyncEv ev)
{
	Sigil::instance().addEvent(ev);
}

void SGLnotifyCF(SglCFEv ev)
{
}

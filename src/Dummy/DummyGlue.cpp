#include "Dummy.h"
#include "DummyGlue.hpp"

void handleMemEvent(const Event<DataAccess>* ev)
{
	doMemEventProcessing(ev->uid);
}

void handleCompEvent(const Event<DataCompute>* ev)
{
	doCompEventProcessing(ev->uid);
}

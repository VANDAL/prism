#include "Sigil2/SglLog.hpp"
#include "EventHandlers.hpp"

namespace STGen
{
	void initThreadLog(TId tid);
	void switchThreadLog(TId tid);

	void logCompEvent(STCompEvent& ev);
	void logCommEdgeEvent();
	void logSyncEvent(TId tid, EId eid, UChar type, Addr addr);
};

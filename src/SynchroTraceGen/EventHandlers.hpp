#ifndef STGEN_EVENTHANDLERS_H
#define STGEN_EVENTHANDLERS_H

#include "Sigil2/SglLog.hpp"

#include "ShadowMemory.hpp"
#include "LogHelpers.hpp"
#include "STEvent.hpp"

namespace STGen
{
	class EventHandlers
	{
		typedef int32_t TId; 
		public:
			EventHandlers();
			~EventHandlers();

			void onSyncEv(SglSyncEv ev);
			void onCompEv(SglCompEv ev);
			void onMemEv(SglMemEv ev);
			void onCxtEv(SglCxtEv ev);

		private:
			void onThreadSwap();
			void onLoad(const SglMemEv& ev_data);
			void onStore(const SglMemEv& ev_data);

		private:
			ShadowMemory shad_mem {};

		public:
			STCompEvent st_comp_ev;
			STCommEvent st_comm_ev;
			STSyncEvent st_sync_ev;
	};
};

#endif

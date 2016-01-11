#ifndef SHADOWMEMORY_H
#define SHADOWMEMORY_H

#include "Sigil2/Primitive.h"
#include "STEvent.hpp"

#include <cstdint>
#include <vector>

/* Shadow Memory tracks 'shadow state' for an address.
 *
 * In SynchroTraceGen, 'shadow state' takes the form of 
 * the most recent thread to read from/write to an address.
 *
 * For further clarification, please read,
 * "How to Shadow Every Byte of Memory Used by a Program"
 * by Nicholas Nethercote and Julian Seward
 */


namespace STGen
{
	constexpr TId SO_UNDEF = -1;

	class ShadowMemory
	{
		public:

		public:
			ShadowMemory(Addr addr_bits = 38, Addr pm_bits = 16);

			void updateWriter(Addr addr, UInt bytes, TId tid, EId event_id);
			void updateReader(Addr addr, UInt bytes, TId tid);
			TId getWriterTID(Addr addr);
			EId getWriterEID(Addr addr);
			TId getReaderTID(Addr addr);

		public:
			/* Configuration */
			const Addr addr_bits;
			const Addr pm_bits;
			const Addr sm_bits;
			const Addr pm_size;
			const Addr sm_size;
			const Addr max_primary_addr;

		private:
			/* Secondary/Primary Maps */
			struct SecondaryMap
			{
				std::vector<TId> last_writers; // Last thread to write to addr
				std::vector<EId>  last_writers_event; // Last event to write to addr
				std::vector<TId>  last_readers; // Last thread to read to addr
			};
			std::vector<SecondaryMap*> PM;
			SecondaryMap DSM;

		private:
			/* Utility Functions */
			SecondaryMap& getSMFromAddr(Addr addr);
			void initSM(SecondaryMap*& SM);
			uint64_t getSMidx(Addr addr) const;
			uint64_t getPMidx(Addr addr) const;
	};
};

#endif

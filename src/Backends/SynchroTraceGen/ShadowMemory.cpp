#include "Sigil2/SigiLog.hpp"
#include "ShadowMemory.hpp"
#include <cassert>
#include <iostream>

namespace STGen
{

void ShadowMemory::updateWriter(Addr addr, UInt bytes, TId tid, EId eid)
{
	for(UInt i=0; i<bytes; ++i)
	{
		Addr curr_addr = addr+i;
		SecondaryMap& sm = getSMFromAddr(curr_addr);

		sm.last_writers[getSMidx(curr_addr)] = tid;
		sm.last_writers_event[getSMidx(curr_addr)] = eid;

		//reset readers on new write
		sm.last_readers[getSMidx(curr_addr)] = SO_UNDEF;
	}
}


void ShadowMemory::updateReader(Addr addr, UInt bytes, TId tid)
{
	for(UInt i=0; i<bytes; ++i)
	{
		Addr curr_addr = addr+i;
		SecondaryMap& sm = getSMFromAddr(curr_addr);
		sm.last_readers[getSMidx(curr_addr)] = tid;
	}
}


TId ShadowMemory::getReaderTID(Addr addr)
{
	return getSMFromAddr(addr).last_readers[getSMidx(addr)];
}


TId ShadowMemory::getWriterTID(Addr addr)
{
	return getSMFromAddr(addr).last_writers[getSMidx(addr)];
}


EId ShadowMemory::getWriterEID(Addr addr)
{
	return getSMFromAddr(addr).last_writers_event[getSMidx(addr)];
}


ShadowMemory::ShadowMemory(Addr addr_bits, Addr pm_bits)
	: addr_bits( addr_bits )
	, pm_bits( pm_bits )
	, sm_bits( addr_bits - pm_bits )
	, pm_size( 1ULL << pm_bits )
	, sm_size( 1ULL << sm_bits )
	, max_primary_addr( (1ULL << addr_bits) - 1 )
{
	DSM = new SecondaryMap;
	DSM->last_readers.resize(sm_size, SO_UNDEF);
	DSM->last_writers.resize(sm_size, SO_UNDEF);
	DSM->last_writers_event.resize(sm_size, SO_UNDEF);

	PM = new std::vector<SecondaryMap*>;
	PM->resize(pm_size, nullptr);
}


ShadowMemory::~ShadowMemory()
{
	delete DSM;
	for(auto SM : *PM)
	{
		if(SM != nullptr)
		{
			delete SM;
		}
	}
	delete PM;
}


///////////////////////////////////////
// Utility Functions
///////////////////////////////////////
inline ShadowMemory::SecondaryMap& ShadowMemory::getSMFromAddr(Addr addr)
{
	if(addr > max_primary_addr)
	{
		char s_addr[32];
		sprintf(s_addr, "0x%lx", addr);

		char s_max[32];
		sprintf(s_max, "0x%lx", max_primary_addr);

		std::string msg("shadow memory max address limit: ");
		msg.append(s_addr).append(" > ").append(s_max);
		SigiLog::fatal(msg);
	}

	SecondaryMap*& SM = (*PM)[getPMidx(addr)];
	if(SM == nullptr)
	{
		SM = new SecondaryMap(*DSM);
	}
	return *SM;
}


inline uint64_t ShadowMemory::getSMidx(Addr addr) const
{
	return addr & ((1ULL<<sm_bits)-1);
}


inline uint64_t ShadowMemory::getPMidx(Addr addr) const
{
	return addr>>sm_bits;
}

}; //end namespace STGen

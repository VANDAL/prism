#include "Sigil2/SigiLog.hpp"
#include "ShadowMemory.hpp"
#include <sstream> // for printing hex vals

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


ShadowMemory::ShadowMemory(Addr addr_bits, Addr pm_bits, Addr max_shad_mem_size)
	: addr_bits( addr_bits )
	, pm_bits( pm_bits )
	, sm_bits( addr_bits - pm_bits )
	, pm_size( 1ULL << pm_bits )
	, sm_size( 1ULL << sm_bits )
	, max_shad_mem_size( max_shad_mem_size )
	, curr_shad_mem_size( 0 ) /* includes DSM */
	, curr_sm_count( 0 ) /* excludes DSM */
{
	assert(addr_bits > 0);
	assert(pm_bits > 0);

	pm_Mbytes = pm_size * sizeof(char*) /(1<<20);
	sm_Mbytes = sm_size * (sizeof(TId)*2 + sizeof(EId)) /(1<<20);

	if(sm_Mbytes < 1)
	{
		SigiLog::fatal("SM size too small - adjust shadow memory configs in source code");
	}
	SigiLog::debug("Shadow Memory PM size: " + std::to_string(pm_Mbytes) + " MB");
	SigiLog::debug("Shadow Memory SM size: " + std::to_string(sm_Mbytes) + " MB");

	DSM = new SecondaryMap;
	DSM->last_readers.resize(sm_size, SO_UNDEF);
	DSM->last_writers.resize(sm_size, SO_UNDEF);
	DSM->last_writers_event.resize(sm_size, SO_UNDEF);

	curr_shad_mem_size += pm_Mbytes;
	curr_shad_mem_size += sm_Mbytes;

	PM = new std::vector<SecondaryMap*>;
	PM->resize(pm_size, nullptr);
}


ShadowMemory::~ShadowMemory()
{
	SigiLog::debug("Shadow Memory approximate size: " + std::to_string(curr_shad_mem_size) + " MB");
	SigiLog::debug("Shadow Memory Secondary Maps used: " + std::to_string(curr_sm_count));

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
	if((addr >> addr_bits) > 0)
	{
		char s_addr[32];
		sprintf(s_addr, "0x%lx", addr);

		std::string msg("shadow memory max address limit: ");
		msg.append(s_addr);

		SigiLog::fatal(msg);
	}

	SecondaryMap*& SM = (*PM)[getPMidx(addr)];
	if(SM == nullptr)
	{
		addSMsize();
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


inline void ShadowMemory::addSMsize()
{
	++curr_sm_count;
	curr_shad_mem_size += sm_Mbytes;
	if(curr_shad_mem_size > max_shad_mem_size)
	{
		SigiLog::fatal("shadow memory size limits exceeded");
	}
}

}; //end namespace STGen

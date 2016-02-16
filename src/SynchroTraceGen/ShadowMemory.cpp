#include "ShadowMemory.hpp"
#include <cassert>

namespace STGen
{
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;

void ShadowMemory::updateWriter(Addr addr, UInt bytes, TId tid, EId eid)
{
	for (UInt i=0; i<bytes; ++i)
	{
		Addr curr_addr = addr+i;
		auto& sm = getSMFromAddr(curr_addr);

		sm->last_writers[getSMidx(curr_addr)] = tid;
		sm->last_writers_event[getSMidx(curr_addr)] = eid;

		//reset readers on new write
		sm->last_readers[getSMidx(curr_addr)] = SO_UNDEF;
	}
}

void ShadowMemory::updateReader(Addr addr, UInt bytes, TId tid)
{
	for (UInt i=0; i<bytes; ++i)
	{
		Addr curr_addr = addr+i;
		auto& sm = getSMFromAddr(curr_addr);
		sm->last_readers[getSMidx(curr_addr)] = tid;
	}
}

TId ShadowMemory::getReaderTID(Addr addr)
{
	return getSMFromAddr(addr)->last_readers[getSMidx(addr)];
}

TId ShadowMemory::getWriterTID(Addr addr) 
{
	return getSMFromAddr(addr)->last_writers[getSMidx(addr)];
}

EId ShadowMemory::getWriterEID(Addr addr)
{
	return getSMFromAddr(addr)->last_writers_event[getSMidx(addr)];
}

ShadowMemory::ShadowMemory(Addr addr_bits, Addr pm_bits)
	: addr_bits( addr_bits )
	, pm_bits( pm_bits )
	, sm_bits( addr_bits - pm_bits )
	, pm_size( 1ULL << pm_bits )
	, sm_size( 1ULL << sm_bits )
	, max_primary_addr( (1ULL << addr_bits) - 1 )
{
	DSM = unique_ptr<SecondaryMap>(new SecondaryMap());
	DSM->last_readers.resize(sm_size, SO_UNDEF);
	DSM->last_writers.resize(sm_size, SO_UNDEF);
	DSM->last_writers_event.resize(sm_size, SO_UNDEF);

	PM = unique_ptr<vector<unique_ptr<SecondaryMap>>>(new vector<unique_ptr<SecondaryMap>>());
	PM->resize(pm_size);
}

///////////////////////////////////////
// Utility Functions 
///////////////////////////////////////
inline const unique_ptr<ShadowMemory::SecondaryMap>& ShadowMemory::getSMFromAddr(Addr addr)
{
	assert( addr <= max_primary_addr );

	unique_ptr<SecondaryMap>& SM = (*PM)[getPMidx(addr)];
	if (SM == nullptr)
	{
		SM = unique_ptr<SecondaryMap>(new SecondaryMap(*DSM));
	}
	return SM;
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

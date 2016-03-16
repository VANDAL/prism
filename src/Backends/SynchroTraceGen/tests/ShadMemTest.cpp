#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "SynchroTraceGen/ShadowMemory.hpp"

#include <stdlib.h>
#include <time.h>
#include <iostream>

using STGen::ShadowMemory;
using TId = STGen::TId;
using EId = STGen::EId;

TEST_CASE( "shadow memory inits readers/writers", "[ShadowMemoryInit]" )
{
	SECTION( "secondary maps and values are INITIALIZED" )
	{
		ShadowMemory sm;
		REQUIRE( sm.getWriterTID(0) == STGen::SO_UNDEF );
		REQUIRE( sm.getWriterEID(0) == STGen::SO_UNDEF );
		REQUIRE( sm.getReaderTID(0) == STGen::SO_UNDEF );

		REQUIRE( sm.getWriterTID(sm.max_primary_addr) == STGen::SO_UNDEF );
		REQUIRE( sm.getWriterEID(sm.max_primary_addr) == STGen::SO_UNDEF );
		REQUIRE( sm.getReaderTID(sm.max_primary_addr) == STGen::SO_UNDEF );

		/* will segfault, max address boundary for default shadow memory
		 * REQUIRE( sm.getReaderTID((1ULL<<38)) == -1 );
		 */
	}
}

TEST_CASE( "shadow memory tracks readers/writers", "[ShadowMemorySetGet]" )
{
	srand(time(NULL));

	SECTION( "setting READERS across SM boundaries" )
	{
		std::cout << std::endl;
		ShadowMemory sm;

		TId tid1 = rand() % 1000;
		Addr addr1 = 0x0000;
		SglMemEv ev1 = {MEM_LOAD, addr1, 4, 0};
		sm.updateReader(ev1.begin_addr, ev1.size, tid1);

		TId tid2 = rand() % 1000;
		Addr addr2 = sm.sm_size-1;
		UInt bytes = 8;
		SglMemEv ev2 = {MEM_LOAD, addr2, bytes, 0};
		sm.updateReader(ev2.begin_addr, ev2.size, tid2);

		REQUIRE( sm.getReaderTID(addr1) == tid1 );

		//This should cross SM boundaries
		REQUIRE( sm.getReaderTID(addr2) == tid2 );
		REQUIRE( sm.getReaderTID(addr2+bytes-1) == tid2 );

		REQUIRE( sm.getReaderTID(addr2+bytes) == STGen::SO_UNDEF );
	}

	SECTION( "setting WRITERS across SM boundaries" )
	{
		ShadowMemory sm;


		TId tid1 = rand() % 1000;
		Addr addr1 = 0x0000;
		SglMemEv ev1 = {MEM_STORE, addr1, 4, 0};
		EId eid1 = rand() % 1000;
		sm.updateWriter(ev1.begin_addr, ev1.size, tid1, eid1);

		TId tid2 = rand() % 1000;
		Addr addr2 = sm.sm_size-1;
		UInt bytes = 8;
		SglMemEv ev2 = {MEM_STORE, addr2, bytes, 0};
		EId eid2 = rand() % 1000;
		sm.updateWriter(ev2.begin_addr, ev2.size, tid2, eid2);

		REQUIRE( sm.getWriterTID(addr1) == tid1 );
		REQUIRE( sm.getWriterEID(addr1) == eid1 );

		//This should cross SM boundaries
		REQUIRE( sm.getWriterTID(addr2) == tid2 );
		REQUIRE( sm.getWriterTID(addr2+bytes-1) == tid2 );
		REQUIRE( sm.getWriterEID(addr2) == eid2 );
		REQUIRE( sm.getWriterEID(addr2+bytes-1) == eid2 );

		//Sanity check
		REQUIRE( sm.getReaderTID(addr2) != tid2 );
		REQUIRE( sm.getReaderTID(addr2) == STGen::SO_UNDEF );
	}

	SECTION( "setting writer clears out reader" ) 
	{
		ShadowMemory sm;

		TId tid1 = 1;
		Addr addr1 = 0x0000;
		EId eid1 = 0xFFFFF;
		SglMemEv load = {MEM_LOAD, addr1, 4, 0};
		SglMemEv store = {MEM_STORE, addr1, 4, 0};

		sm.updateReader(load.begin_addr, load.size, tid1);
		REQUIRE( sm.getReaderTID(addr1) == tid1 );
		REQUIRE( sm.getWriterTID(addr1) == STGen::SO_UNDEF );

		sm.updateWriter(store.begin_addr, store.size, tid1, eid1);
		REQUIRE( sm.getReaderTID(addr1) == STGen::SO_UNDEF );
		REQUIRE( sm.getWriterTID(addr1) == tid1 );
		REQUIRE( sm.getWriterEID(addr1) == eid1 );
	}

	SECTION( "setting multiple readers" )
	{
		//TODO
	}
}



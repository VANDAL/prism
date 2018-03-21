#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <stdlib.h>
#include <time.h>

#include "SynchroTraceGen/STShadowMemory.hpp"

using STGen::STShadowMemory;
using STGen::TID;
using STGen::EID;

TEST_CASE("shadow memory inits readers/writers", "[ShadowMemoryInit]")
{
    SECTION("secondary maps and values are INITIALIZED")
    {
        STShadowMemory sm;
        REQUIRE(sm.getWriterTID(0) == STGen::SO_UNDEF);
        REQUIRE(sm.getWriterEID(0) == 0);

        for(size_t i = 0; i < STGen::MAX_THREADS; ++i)
        {
            REQUIRE(sm.isReaderTID(0, i) == false);
        }
    }
}

TEST_CASE("shadow memory tracks readers/writers", "[ShadowMemorySetGet]")
{
    srand(time(NULL));

    SECTION("setting READERS across SM boundaries")
    {
        std::cout << std::endl;
        STShadowMemory sm;

        TID tid1 = rand() % STGen::MAX_THREADS;
        Addr addr1 = 0x0000;
        PrismMemEv ev1 = {addr1, 4, PRISM_MEM_LOAD,};
        sm.updateReader(ev1.begin_addr, ev1.size, tid1);

        TID tid2 = rand() % STGen::MAX_THREADS;
        Addr addr2 = sm.sm.sm_size - 1;
        ByteCount bytes = 8;
        PrismMemEv ev2 = {addr2, bytes, PRISM_MEM_LOAD,};
        sm.updateReader(ev2.begin_addr, ev2.size, tid2);

        REQUIRE(sm.isReaderTID(addr1, tid1) == true);

        //This should cross SM boundaries
        REQUIRE(sm.isReaderTID(addr2, tid2) == true);
        REQUIRE(sm.isReaderTID(addr2 + bytes - 1, tid2) == true);
        REQUIRE(sm.isReaderTID(addr2 + bytes, tid2) == false);
    }

    SECTION("setting WRITERS across SM boundaries")
    {
        STShadowMemory sm;

        TID tid1 = rand() % STGen::MAX_THREADS;
        Addr addr1 = 0x0000;
        PrismMemEv ev1 = {addr1, 4, PRISM_MEM_STORE,};
        EID eid1 = rand() % 1000;
        sm.updateWriter(ev1.begin_addr, ev1.size, tid1, eid1);

        TID tid2 = rand() % STGen::MAX_THREADS;
        Addr addr2 = sm.sm.sm_size - 1;
        ByteCount bytes = 8;
        PrismMemEv ev2 = {addr2, bytes, PRISM_MEM_STORE,};
        EID eid2 = rand() % 1000;
        sm.updateWriter(ev2.begin_addr, ev2.size, tid2, eid2);

        REQUIRE(sm.getWriterTID(addr1) == tid1);
        REQUIRE(sm.getWriterEID(addr1) == eid1);

        //This should cross SM boundaries
        REQUIRE(sm.getWriterTID(addr2) == tid2);
        REQUIRE(sm.getWriterTID(addr2 + bytes - 1) == tid2);
        REQUIRE(sm.getWriterEID(addr2) == eid2);
        REQUIRE(sm.getWriterEID(addr2 + bytes - 1) == eid2);

        //Sanity check
        REQUIRE(sm.isReaderTID(addr2, tid2) == false);
    }

    SECTION("setting writer clears out reader")
    {
        STShadowMemory sm;

        TID tid1 = 1;
        Addr addr1 = 0x0000;
        EID eid1 = 0xFFFFF;
        PrismMemEv load = {addr1, 4, PRISM_MEM_LOAD,};
        PrismMemEv store = {addr1, 4, PRISM_MEM_STORE,};

        sm.updateReader(load.begin_addr, load.size, tid1);
        REQUIRE(sm.isReaderTID(addr1, tid1) == true);
        REQUIRE(sm.getWriterTID(addr1) == STGen::SO_UNDEF);

        sm.updateWriter(store.begin_addr, store.size, tid1, eid1);
        REQUIRE(sm.isReaderTID(addr1, tid1) == false);
        REQUIRE(sm.getWriterTID(addr1) == tid1);
        REQUIRE(sm.getWriterEID(addr1) == eid1);
    }

    SECTION("setting multiple readers")
    {
        //TODO
    }

    SECTION("thread safety of setting/resetting multiple readers")
    {
        //TODO
    }
}



#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <stdlib.h>
#include <time.h>

#include "SynchroTraceGen/ShadowMemory.hpp"

using STGen::ShadowMemory;
using STGen::TID;
using STGen::EID;

TEST_CASE("shadow memory inits readers/writers", "[ShadowMemoryInit]")
{
    SECTION("secondary maps and values are INITIALIZED")
    {
        ShadowMemory sm;
        REQUIRE(sm.getWriterTID(0) == STGen::SO_UNDEF);
        REQUIRE(sm.getWriterEID(0) == STGen::SO_UNDEF);
        REQUIRE(sm.isReaderTID(0, STGen::SO_UNDEF) == true);
    }
}

TEST_CASE("shadow memory tracks readers/writers", "[ShadowMemorySetGet]")
{
    srand(time(NULL));

    SECTION("setting READERS across SM boundaries")
    {
        std::cout << std::endl;
        ShadowMemory sm;

        TID tid1 = rand() % 1000;
        Addr addr1 = 0x0000;
        SglMemEv ev1 = {SGLPRIM_MEM_LOAD, addr1, 4, 0};
        sm.updateReader(ev1.begin_addr, ev1.size, tid1);

        TID tid2 = rand() % 1000;
        Addr addr2 = sm.sm_size - 1;
        ByteCount bytes = 8;
        SglMemEv ev2 = {SGLPRIM_MEM_LOAD, addr2, bytes, 0};
        sm.updateReader(ev2.begin_addr, ev2.size, tid2);

        REQUIRE(sm.isReaderTID(addr1, tid1) == true);

        //This should cross SM boundaries
        REQUIRE(sm.isReaderTID(addr2, tid2) == true);
        REQUIRE(sm.isReaderTID(addr2 + bytes - 1, tid2) == true);

        REQUIRE(sm.isReaderTID(addr2 + bytes, STGen::SO_UNDEF) == true);
    }

    SECTION("setting WRITERS across SM boundaries")
    {
        ShadowMemory sm;

        TID tid1 = rand() % 1000;
        Addr addr1 = 0x0000;
        SglMemEv ev1 = {SGLPRIM_MEM_STORE, addr1, 4, 0};
        EID eid1 = rand() % 1000;
        sm.updateWriter(ev1.begin_addr, ev1.size, tid1, eid1);

        TID tid2 = rand() % 1000;
        Addr addr2 = sm.sm_size - 1;
        ByteCount bytes = 8;
        SglMemEv ev2 = {SGLPRIM_MEM_STORE, addr2, bytes, 0};
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
        REQUIRE(sm.isReaderTID(addr2, STGen::SO_UNDEF) == true);
    }

    SECTION("setting writer clears out reader")
    {
        ShadowMemory sm;

        TID tid1 = 1;
        Addr addr1 = 0x0000;
        EID eid1 = 0xFFFFF;
        SglMemEv load = {SGLPRIM_MEM_LOAD, addr1, 4, 0};
        SglMemEv store = {SGLPRIM_MEM_STORE, addr1, 4, 0};

        sm.updateReader(load.begin_addr, load.size, tid1);
        REQUIRE(sm.isReaderTID(addr1, tid1) == true);
        REQUIRE(sm.getWriterTID(addr1) == STGen::SO_UNDEF);

        sm.updateWriter(store.begin_addr, store.size, tid1, eid1);
        REQUIRE(sm.isReaderTID(addr1, STGen::SO_UNDEF) == true);
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



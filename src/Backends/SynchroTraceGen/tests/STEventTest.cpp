#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "SynchroTraceGen/STEvent.hpp"
#include "SynchroTraceGen/EventHandlers.hpp"

using STGen::EventHandlers;
using STGen::STCompEvent;
using STGen::STCommEvent;

TEST_CASE("SynchroTrace Events", "[STEvent init]")
{

    SECTION("Compute Event")
    {
        //TODO
    }

    SECTION("Communication Event")
    {
        //TODO
    }

    SECTION("Synchronization Event")
    {
        //TODO
    }
}

//TEST_CASE("Compute Event Handling", "[STCompEvent]")
//{
//    typedef std::vector<std::pair<Addr, Addr>> Ranges;
//    STGen::EventHandlers handlers;
//    UInt &iop_cnt = handlers.st_comp_ev.iop_cnt;
//    UInt &flop_cnt = handlers.st_comp_ev.flop_cnt;
//    UInt &load_cnt = handlers.st_comp_ev.load_cnt;
//    UInt &store_cnt = handlers.st_comp_ev.store_cnt;
//    Ranges &load_ranges = handlers.st_comp_ev.loads_unique.ranges;;
//    Ranges &store_ranges = handlers.st_comp_ev.stores_unique.ranges;;
//
//    //SglCompEv ev = {type, arity, op, size (width)};
//    //SglMemEv ev = {type, addr, size, alignment (unused)};
//
//    SglSyncEv thread0 = {SyncType::SYNC_SWAP, 0};
//
//    SglCompEv ev1 = {COMP_IOP, COMP_BINARY, COMP_MOV, 4};
//    SglCompEv ev2 = {COMP_IOP, COMP_BINARY, COMP_SUB, 4};
//    SglCompEv ev3 = {COMP_IOP, COMP_BINARY, COMP_ADD, 4};
//
//    SglMemEv  ev4 = {MEM_STORE, 0x0000, 4, 0};
//
//    SglCompEv ev5 = {COMP_FLOP, COMP_TERNARY, COMP_MULT, 4};
//
//    SglMemEv  ev6 = {MEM_STORE, 0x0003, 4, 0};
//
//    SglCompEv ev7 = {COMP_FLOP, COMP_TERNARY, COMP_MULT, 4};
//    SglCompEv ev8 = {COMP_IOP, COMP_QUARTERNARY, COMP_ADD, 4};
//
//    SglMemEv  ev9 = {MEM_LOAD, 0x0000, 4, 0};
//
//    SglSyncEv thread1 = {SyncType::SYNC_SWAP, 1};
//
//    SglMemEv ev10 = {MEM_LOAD, 0x0005, 1, 0};//read from thread0 inside thread1
//
//
//    SECTION("Building up a ST Compute Event")
//    {
//
//        STEvent::setThread(0);
//        REQUIRE(iop_cnt == 0);
//        REQUIRE(flop_cnt == 0);
//        REQUIRE(load_cnt == 0);
//        REQUIRE(store_cnt == 0);
//
//        handlers.onSyncEv(thread0);
//        handlers.onCompEv(ev1);
//        handlers.onCompEv(ev2);
//        handlers.onCompEv(ev3);
//        handlers.onMemEv(ev4);
//        handlers.onCompEv(ev5);
//        handlers.onMemEv(ev6);
//        handlers.onCompEv(ev7);
//        handlers.onCompEv(ev8);
//        handlers.onMemEv(ev9);
//
//        REQUIRE(iop_cnt == 4);
//        REQUIRE(flop_cnt == 2);
//        REQUIRE(load_cnt == 4);
//        REQUIRE(store_cnt == 8);
//        REQUIRE(load_ranges.size() == 1);
//        REQUIRE(store_ranges.size() == 2);
//    }
//
//    SECTION("Flushing on a thread swap")
//    {
//        handlers.onSyncEv(thread0);
//
//        handlers.onCompEv(ev1);
//        handlers.onCompEv(ev2);
//        handlers.onCompEv(ev3);
//        handlers.onMemEv(ev4);
//        handlers.onCompEv(ev5);
//        handlers.onMemEv(ev6);
//        handlers.onCompEv(ev7);
//        handlers.onCompEv(ev8);
//        handlers.onMemEv(ev9);
//
//        REQUIRE(iop_cnt == 4);
//        REQUIRE(flop_cnt == 2);
//        REQUIRE(load_cnt == 4);
//        REQUIRE(store_cnt == 8);
//        REQUIRE(load_ranges.size() == 1);
//        REQUIRE(store_ranges.size() == 2);
//
//        handlers.onSyncEv(thread1);
//
//        REQUIRE(iop_cnt == 0);
//        REQUIRE(flop_cnt == 0);
//        REQUIRE(load_cnt == 0);
//        REQUIRE(store_cnt == 0);
//        REQUIRE(load_ranges.size() == 0);
//        REQUIRE(store_ranges.size() == 0);
//    }
//
//    SECTION("Flushing on an inter-thread communication event")
//    {
//        handlers.onSyncEv(thread0);
//
//        handlers.onCompEv(ev1);
//        handlers.onCompEv(ev2);
//        handlers.onCompEv(ev3);
//        handlers.onMemEv(ev4);
//        handlers.onCompEv(ev5);
//        handlers.onMemEv(ev6);
//        handlers.onCompEv(ev7);
//        handlers.onCompEv(ev8);
//        handlers.onMemEv(ev9);
//
//        REQUIRE(iop_cnt == 4);
//        REQUIRE(flop_cnt == 2);
//        REQUIRE(load_cnt == 4);
//        REQUIRE(store_cnt == 8);
//        REQUIRE(load_ranges.size() == 1);
//        REQUIRE(store_ranges.size() == 2);
//
//        handlers.onSyncEv(thread1);
//        handlers.onCompEv(ev1);
//        handlers.onCompEv(ev2);
//        handlers.onCompEv(ev3);
//
//        REQUIRE(iop_cnt == 3);
//        REQUIRE(flop_cnt == 0);
//        REQUIRE(load_cnt == 0);
//        REQUIRE(store_cnt == 0);
//        REQUIRE(load_ranges.size() == 0);
//        REQUIRE(store_ranges.size() == 0);
//
//        handlers.onMemEv(ev10);
//
//        REQUIRE(iop_cnt == 0);
//    }
//
//    SECTION("Flushing on a specific number of events (split-by-n)")
//    {
//        //TODO
//    }
//}

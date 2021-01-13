#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "CoreEventLoop.hpp"
#include "EventBuffer.h"

#include <iostream>

#define GET_EV_TY(buf) ((unsigned char)(*buf >> 5))

size_t set_mem_ev(unsigned char* buf) {
    SET_EV_TYPE(buf, EventTypeEnum::PRISM_EVENTTYPE_MEM);
    SET_EV_MEM_TYPE(buf, MemTypeEnum::PRISM_MEM_STORE);
    SET_EV_MEM_SIZE(buf, MemSizeEnum::PRISM_MEM_16);
    SET_EV_MEM_ADDR(buf, 0xDEADBEEF'BAADF00D);
    SET_EV_MEM_ID_AFTER_ADDR(buf, 12);
    return 9;
}

size_t set_comp_ev(unsigned char* buf) {
    SET_EV_TYPE(buf, EventTypeEnum::PRISM_EVENTTYPE_COMP);
    SET_EV_COMP_FMT(buf, CompCostTypeEnum::PRISM_COMP_IOP);
    SET_EV_COMP_SZ(buf, MemSizeEnum::PRISM_MEM_128);

    SET_EV_COMP_TYPE(buf, CompCostOpEnum::PRISM_COMP_MULT);
    SET_EV_COMP_ARITY(buf, 1); // 1 + 1 = 2

    SET_EV_COMP_ID(buf, 0, 12);
    SET_EV_COMP_ID(buf, 1, 111);
    return 4;
}

size_t set_sync_ev(unsigned char* buf) {
    SET_EV_TYPE(buf, EventTypeEnum::PRISM_EVENTTYPE_SYNC);
    SET_EV_SYNC_TY(buf, SyncTypeEnum::PRISM_SYNC_CREATE);

    // the actual data depends on the sync type
    // For a create, it's a 'thread id'
    *(uintptr_t*)(buf+1) = {101};
    return 9;
}

constexpr const char name[] = "aCoolFunctionBeingProfiled";
constexpr auto name_len_wo_null = sizeof(name) - 1;
size_t set_cxt_ev(unsigned char* buf) {
    SET_EV_CXT_W_NAME(buf, name_len_wo_null, name, CxtTypeEnum::PRISM_CXT_FUNC_ENTER);
    return (2 + name_len_wo_null);
}


class Printer : public BackendIface {
    virtual auto onSyncEv(const prism::SyncEvent &ev) -> void override {
        std::cout << "sync event\n";
    }
    virtual auto onCompEv(const prism::CompEvent &ev) -> void override {
        std::cout << "comp event\n";
    }
    virtual auto onMemEv(const prism::MemEvent &ev) -> void override {
        std::cout << "mem event\n";
    }
    virtual auto onCxtEv(const prism::CxtEvent &ev) -> void override {
        std::cout << "cxt event: " << ev.type() << " \n";
    }
};


TEST_CASE("Single Event Parsing", "[EventBuffer]")
{
    unsigned char mem_buf[9] = {0};
    unsigned char comp_buf[4] = {0};
    unsigned char sync_buf[9] = {0};
    unsigned char cxt_buf[1 + 1 + name_len_wo_null];

    set_mem_ev(mem_buf);
    set_comp_ev(comp_buf);
    set_sync_ev(sync_buf);
    set_cxt_ev(cxt_buf);

    SECTION("1 Memory Event")
    {
        unsigned char* buf = mem_buf;
        prism::MemEvent ev{buf};

        REQUIRE(GET_EV_TY(buf) == EventTypeEnum::PRISM_EVENTTYPE_MEM);
        REQUIRE(ev.isStore());
        REQUIRE(ev.accessed_bytes() == 2);
        REQUIRE(ev.addr() == 0xADBEEF'BAADF00D);
        REQUIRE(ev.id() == 12);
    }

    SECTION("1 Compute Event")
    {
        unsigned char *buf = comp_buf;
        prism::CompEvent ev{buf};

        REQUIRE(GET_EV_TY(buf) == EventTypeEnum::PRISM_EVENTTYPE_COMP);
        REQUIRE(ev.isIOP());
        REQUIRE(ev.width_bytes() == 16);
        REQUIRE(ev.type() == CompCostOpEnum::PRISM_COMP_MULT);
        REQUIRE(ev.arity() == 2);
        REQUIRE(ev.id(0) == 12);
        REQUIRE(ev.id(1) == 111);

    }

    SECTION("1 Sync Event")
    {
        unsigned char* buf = sync_buf;
        prism::SyncEvent ev{buf};

        REQUIRE(GET_EV_TY(buf) == EventTypeEnum::PRISM_EVENTTYPE_SYNC);
        REQUIRE(ev.type() == SyncTypeEnum::PRISM_SYNC_CREATE);
        REQUIRE(ev.data() == 101);
    }

    SECTION("1 Context Event")
    {
        unsigned char* buf = cxt_buf;
        prism::CxtEvent ev{buf};

        REQUIRE(GET_EV_TY(buf) == EventTypeEnum::PRISM_EVENTTYPE_CXT);
        REQUIRE(ev.type() == CxtTypeEnum::PRISM_CXT_FUNC_ENTER);
        REQUIRE(memcmp(name, ev.cxt_name(), ev.cxt_name_len()) == 0);

    }

}

TEST_CASE("Event Stream w/ End", "[EventBufferStream]")
{
    /**
     * Check the event loop that runs until it hits the last event.
     * 
     * 1 of each event:
     * 0:  CfgMem: 2 byte,
     * 2:  CfgComp: 2 byte,
     * 4:  Mem: 9 bytes,
     * 13: Comp: 2 + N(2)  = 4 bytes
     * 17: Sync: 1 + N(8) = 9 bytes -- create
     * 26: Cxt: 1 + N(27) = 28 bytes -- function
     * 54: Cxt: 1 + N(7)  = 8 bytes -- basic block
     * 62: Cxt: 1 + N(7) = 8 bytes -- instruction
     * 70: CfgMem: 2 byte,
     * 72: Mem: 2 bytes,
     * 74: End: 1 byte
     * 
     * Total: 75 bytes
     *      : 11 events
     */
    unsigned char buf[75] = {0};
    unsigned char *ptr = buf;

    SET_EV_CFG_ALL(ptr, EventTypeEnum::PRISM_EVENTTYPE_MEM, 0b1111'0000);
    ptr += 2;

    SET_EV_CFG_ALL(ptr, EventTypeEnum::PRISM_EVENTTYPE_COMP, 0b1111'1000);
    ptr += 2;

    ptr += set_mem_ev(ptr);
    ptr += set_comp_ev(ptr);
    ptr += set_sync_ev(ptr);
    ptr += set_cxt_ev(ptr);

    SET_EV_CXT_W_ID(ptr, 333, CxtTypeEnum::PRISM_CXT_BB);
    ptr += sizeof(uintptr_t);

    SET_EV_CXT_W_ID(ptr, 111, CxtTypeEnum::PRISM_CXT_INSTR);
    ptr += sizeof(uintptr_t);

    SET_EV_CFG_ALL(ptr, EventTypeEnum::PRISM_EVENTTYPE_MEM, 0b1101'0000);
    ptr += 2;

    SET_EV_TYPE(ptr, EventTypeEnum::PRISM_EVENTTYPE_MEM);
    SET_EV_MEM_TYPE(ptr, MemTypeEnum::PRISM_MEM_LOAD);
    SET_EV_MEM_SIZE(ptr, MemSizeEnum::PRISM_MEM_128);
    SET_EV_MEM_ID(ptr, 90);
    ptr += 2;

    SET_EV_END(ptr);
    ptr += 1;

    REQUIRE(ptr - buf == 75);
    Printer printer{};
    prism::EventConfigState cfg;
    size_t num_events = prism::flushToBackend(printer, cfg, buf);
    REQUIRE(num_events == 11);
}


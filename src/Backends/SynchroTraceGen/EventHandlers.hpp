#ifndef STGEN_EVENTHANDLERS_H
#define STGEN_EVENTHANDLERS_H

#include "Sigil2/Backends.hpp"
#include "ThreadContext.hpp"

namespace STGen
{

/* Sigil2 hooks */
void onParse(Args args);
void onExit();

class EventHandlers : public BackendIface
{
  public:
    EventHandlers() {}
    EventHandlers(const EventHandlers &) = delete;
    EventHandlers &operator=(const EventHandlers &) = delete;
    ~EventHandlers();

    /* Sigil2 event hooks */
    virtual auto onSyncEv(const SglSyncEv &ev) -> void override;
    virtual auto onCompEv(const SglCompEv &ev) -> void override;
    virtual auto onMemEv(const SglMemEv &ev) -> void override;
    virtual auto onCxtEv(const SglCxtEv &ev) -> void override;

  private:
    /* helpers */
    auto onSwapTCxt(TID newTID) -> void;
    auto onCreate(Addr data) -> void;
    auto onBarrier(Addr data) -> void;
    auto convertAndFlush(SyncType type, Addr data) -> void;

    std::unordered_map<TID, ThreadContext> tcxts;
    TID currentTID{SO_UNDEF};
    ThreadContext *cachedTCxt{nullptr};
};

}; //end namespace STGen

#endif

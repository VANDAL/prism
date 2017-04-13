#ifndef STGEN_EVENTHANDLERS_H
#define STGEN_EVENTHANDLERS_H

#include "Core/Backends.hpp"
#include "ThreadContext.hpp"

namespace STGen
{

void onParse(Args args);
void onExit();
/* Sigil2 hooks */

class EventHandlers : public BackendIface
{
  public:
    EventHandlers() {}
    EventHandlers(const EventHandlers &) = delete;
    EventHandlers &operator=(const EventHandlers &) = delete;
    virtual ~EventHandlers() override;

    virtual auto onSyncEv(const SglSyncEvWrapper &ev) -> void override;
    virtual auto onCompEv(const SglCompEvWrapper &ev) -> void override;
    virtual auto onMemEv(const SglMemEvWrapper &ev) -> void override;
    virtual auto onCxtEv(const SglCxtEvWrapper &ev) -> void override;
    /* Sigil2 event hooks */

  private:
    auto onSwapTCxt(TID newTID) -> void;
    auto onCreate(Addr data) -> void;
    auto onBarrier(Addr data) -> void;
    auto convertAndFlush(SyncType type, Addr data) -> void;
    /* helpers */

    std::unordered_map<TID, std::unique_ptr<ThreadContext>> tcxts;
    TID currentTID{SO_UNDEF};
    ThreadContext *cachedTCxt{nullptr};
};

}; //end namespace STGen

#endif

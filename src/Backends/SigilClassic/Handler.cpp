#include "Handler.hpp"

namespace SigilClassic
{

auto Handler::onSyncEv(const SglSyncEv &ev) -> void
{
    /* save the current entity so that it can
     * resume when the thread switches back */
    if(ev.type == SyncTypeEnum::SGLPRIM_SYNC_SWAP)
        cxt.setThreadContext(ev.id);
}


auto Handler::onCompEv(const SglCompEv &ev) -> void
{
    /* aggregate compute costs for the current entity */
    switch(ev.type)
    {
      case CompCostTypeEnum::SGLPRIM_COMP_IOP:
        cxt.incrIOPCost();
        break;
      case CompCostTypeEnum::SGLPRIM_COMP_FLOP:
        cxt.incrFLOPCost();
        break;
      default:
        break;
    }
}


auto Handler::onMemEv(const SglMemEv &ev) -> void
{
    /* - check shadow memory
     * - update the metadata for the current entity */
    switch(ev.type)
    {
      case MemTypeEnum::SGLPRIM_MEM_LOAD:
        cxt.monitorRead(ev.begin_addr, ev.size);
        break;
      case MemTypeEnum::SGLPRIM_MEM_STORE:
        cxt.monitorWrite(ev.begin_addr, ev.size);
        break;
      default:
        break;
    }
}


auto Handler::onCxtEv(const SglCxtEv &ev) -> void
{
    /* Use function contexts as Sigil1 'entities' */
    switch(ev.type)
    {
      case CxtTypeEnum::SGLPRIM_CXT_FUNC_ENTER:
        cxt.enterEntity(ev.name);
        break;
      case CxtTypeEnum::SGLPRIM_CXT_FUNC_EXIT:
        cxt.exitEntity();
        break;
      default:
        break;
    }
}


}; //end namespace SigilClassic

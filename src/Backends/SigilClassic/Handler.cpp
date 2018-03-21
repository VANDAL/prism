#include "Handler.hpp"

namespace SigilClassic
{

auto Handler::onSyncEv(const prism::SyncEvent &ev) -> void
{
    /* save the current entity so that it can
     * resume when the thread switches back */
    auto syncType = ev.type();
    auto syncID = ev.data();

    if(syncType == SyncTypeEnum::PRISM_SYNC_SWAP)
        cxt.setThreadContext(syncID);
}


auto Handler::onCompEv(const prism::CompEvent &ev) -> void
{
    /* aggregate compute costs for the current entity */
    if (ev.isIOP())
        cxt.incrIOPCost();
    else if (ev.isFLOP())
        cxt.incrFLOPCost();
}


auto Handler::onMemEv(const prism::MemEvent &ev) -> void
{
    /* - check shadow memory
     * - update the metadata for the current entity */
    if (ev.isLoad())
        cxt.monitorRead(ev.addr(), ev.bytes());
    else if (ev.isStore())
        cxt.monitorWrite(ev.addr(), ev.bytes());
}


auto Handler::onCxtEv(const prism::CxtEvent &ev) -> void
{
    /* Use function contexts as Sigil1 'entities' */
    switch(ev.type())
    {
      case CxtTypeEnum::PRISM_CXT_FUNC_ENTER:
        cxt.enterEntity(ev.getName());
        break;
      case CxtTypeEnum::PRISM_CXT_FUNC_EXIT:
        cxt.exitEntity();
        break;
      default:
        break;
    }
}


}; //end namespace SigilClassic

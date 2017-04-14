#ifndef SIGILCLASSIC_HANDLER_H
#define SIGILCLASSIC_HANDLER_H

#include "Core/Backends.hpp"
#include "SigilClassic.hpp"

namespace SigilClassic
{

/* interface to Sigil2 */
class Handler : public BackendIface
{
    virtual auto onSyncEv(const sigil2::SyncEvent &ev) -> void override;
    virtual auto onCompEv(const sigil2::CompEvent &ev) -> void override;
    virtual auto onMemEv(const sigil2::MemEvent &ev) -> void override;
    virtual auto onCxtEv(const sigil2::CxtEvent &ev) -> void override;

    SigilContext cxt;
};

}; //end namespace SigilClassic

#endif

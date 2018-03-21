#ifndef SIGILCLASSIC_HANDLER_H
#define SIGILCLASSIC_HANDLER_H

#include "Core/Backends.hpp"
#include "SigilClassic.hpp"

namespace SigilClassic
{

/* interface to Sigil2 */
class Handler : public BackendIface
{
    virtual auto onSyncEv(const prism::SyncEvent &ev) -> void override;
    virtual auto onCompEv(const prism::CompEvent &ev) -> void override;
    virtual auto onMemEv(const prism::MemEvent &ev) -> void override;
    virtual auto onCxtEv(const prism::CxtEvent &ev) -> void override;

    SigilContext cxt;
};

}; //end namespace SigilClassic

#endif

#ifndef SIGILCLASSIC_HANDLER_H
#define SIGILCLASSIC_HANDLER_H

#include "Sigil2/Backends.hpp"
#include "SigilClassic.hpp"

namespace SigilClassic
{

/* interface to Sigil2 */
class Handler : public BackendIface
{
    virtual auto onSyncEv(const SglSyncEv &ev) -> void override;
    virtual auto onCompEv(const SglCompEv &ev) -> void override;
    virtual auto onMemEv(const SglMemEv &ev) -> void override;
    virtual auto onCxtEv(const SglCxtEv &ev) -> void override;

    SigilContext cxt;
};


}; //end namespace SigilClassic

#endif

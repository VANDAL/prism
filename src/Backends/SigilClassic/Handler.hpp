#ifndef SIGILCLASSIC_HANDLER_H
#define SIGILCLASSIC_HANDLER_H

#include "Core/Backends.hpp"
#include "SigilClassic.hpp"

namespace SigilClassic
{

/* interface to Sigil2 */
class Handler : public BackendIface
{
    virtual auto onSyncEv(const SglSyncEvWrapper &ev) -> void override;
    virtual auto onCompEv(const SglCompEvWrapper &ev) -> void override;
    virtual auto onMemEv(const SglMemEvWrapper &ev) -> void override;
    virtual auto onCxtEv(const SglCxtEvWrapper &ev) -> void override;

    SigilContext cxt;
};

}; //end namespace SigilClassic

#endif

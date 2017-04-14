#ifndef SIMPLECOUNT_H
#define SIMPLECOUNT_H

#include "Core/Backends.hpp"

namespace SimpleCount
{

/* interface to Sigil2 */
class Handler : public BackendIface
{
    virtual auto onSyncEv(const sigil2::SyncEvent &ev) -> void override;
    virtual auto onCompEv(const sigil2::CompEvent &ev) -> void override;
    virtual auto onMemEv(const sigil2::MemEvent &ev) -> void override;
    virtual auto onCFEv(const SglCFEv &ev) -> void override;
    virtual auto onCxtEv(const sigil2::CxtEvent &ev) -> void override;

    unsigned long mem_cnt;
    unsigned long comp_cnt;
    unsigned long sync_cnt;
    unsigned long cf_cnt;
    unsigned long cxt_cnt;

  public:
    virtual ~Handler() override;
};

auto cleanup() -> void;

}; //end namespace SimpleCount

#endif

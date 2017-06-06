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

    unsigned long read_cnt{0};
    unsigned long write_cnt{0};
    unsigned long mem_cnt{0};

    unsigned long iop_cnt{0};
    unsigned long flop_cnt{0};
    unsigned long comp_cnt{0};

    unsigned long swap_cnt{0};
    unsigned long sync_cnt{0};
    unsigned long spawn_cnt{0};
    unsigned long join_cnt{0};
    unsigned long lock_cnt{0};
    unsigned long unlock_cnt{0};
    unsigned long barrier_cnt{0};
    unsigned long wait_cnt{0};
    unsigned long sig_cnt{0};
    unsigned long broad_cnt{0};

    unsigned long cf_cnt{0};

    unsigned long instr_cnt{0};
    unsigned long cxt_cnt{0};

  public:
    virtual ~Handler() override;
};

auto cleanup() -> void;

}; //end namespace SimpleCount

#endif

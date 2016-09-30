#ifndef SIMPLECOUNT_H
#define SIMPLECOUNT_H

#include "Sigil2/Sigil.hpp"

namespace SimpleCount
{

/* interface to Sigil2 */
class Handler : public Backend
{
    virtual auto onSyncEv(const SglSyncEv &ev) -> void override;
    virtual auto onCompEv(const SglCompEv &ev) -> void override;
    virtual auto onMemEv(const SglMemEv &ev) -> void override;
    virtual auto onCFEv(const SglCFEv &ev) -> void override;
    virtual auto onCxtEv(const SglCxtEv &ev) -> void override;

    unsigned long mem_cnt;
    unsigned long comp_cnt;
    unsigned long sync_cnt;
    unsigned long cf_cnt;
    unsigned long cxt_cnt;

  public:
    ~Handler();
};

auto cleanup() -> void;

}; //end namespace SimpleCount

#endif

#ifndef _SGLTEST_GEN_H
#define _SGLTEST_GEN_H

#include "EventPrimitive.h"

class WorkloadGenerator
{
  public:
    WorkloadGenerator();

    SglMemEv     getRandMemEvent();
    SglCompEv    getRandCompEvent();
    SglCFEv      getRandCFEvent();
    SglCxtEv     getRandCxtEvent();
    SglSyncEv    getRandSyncEvent();

  private:
    void onMemEvent(SglMemEv ev);
    void onCompEvent(SglCompEv ev);
    void onCFEvent(SglCFEv ev);
    void onCxtEvent(SglCxtEv ev);
    void onSyncEvent(SglSyncEv ev);
};

#endif

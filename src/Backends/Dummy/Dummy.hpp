#ifndef _BE_DUMMY_H_
#define _BE_DUMMY_H_

#include "Sigil2/Primitive.h"

namespace dummy
{
void countMems(SglMemEv ev);
void countComps(SglCompEv ev);
void countSyncs(SglSyncEv ev);
void cleanup();
};

#endif

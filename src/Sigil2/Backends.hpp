#ifndef SIGIL_BACKEND_H
#define SIGIL_BACKEND_H

#include "Primitive.h"

//TODO clean up implementation,
//this shouldn't need a whole separate file

/* Interface for Sigil2 backends.
 *
 * Each backend provides a handler for
 * each Sigil2 event type, and what to do on
 * exit */
class Backend
{
  public:
    virtual void onMemEv (const SglMemEv &)  {}
    virtual void onCompEv(const SglCompEv &) {}
    virtual void onSyncEv(const SglSyncEv &) {}
    virtual void onCxtEv (const SglCxtEv &)  {}
    virtual void onCFEv  (const SglCFEv &)   {}
};


#endif

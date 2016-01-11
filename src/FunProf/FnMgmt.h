#ifndef _FN_MGR_H
#define _FN_MGR_H

#include "Sigil.h"
#include "SigilData.h"

/* Manages and tracks the function context state, including sets 
 * of contexts between threads, and the current function/function context.
 *
 * That is, if the current thread being tracked is swapped, then the current
 * function being tracked has to change with it. This specific behavior is
 * managed in the thread manager. The function manager will simply track ALL
 * function contexts, and leaves it up to the thread manager to know which to
 * swap and when to swap.
 */

/* Sigil tracks the state of functions in the client program by getting
 * updated from an tool interface when specific events such as function
 * calls and function returns occur. */
void handleFnCall(UInt32 fid, const char* fname);
void handleFnReturn();

#define MAX_FNS UINT32_MAX

//Get 
FnCxtNode* getCurrFnCxt();
ClientFnData* getClientFn(UInt32 fid);

#endif

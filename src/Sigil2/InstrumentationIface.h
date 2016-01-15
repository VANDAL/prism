#ifndef SGL_INSTRUMENTATIONIFACE_H
#define SGL_INSTRUMENTATIONIFACE_H

#include "Primitive.h"

/*
 * This file should be included in Sigil instrumentation tools. The tools hook
 * into Sigil by building up event primitives as they are detected and then
 * passing the primitives to Sigil via the API calls declared here.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Memory access */
void SGLnotifyMem(SglMemEv ev);

/* Compute event */
void SGLnotifyComp(SglCompEv ev);

/* Context Marker */
void SGLnotifyCxt(SglCxtEv ev);

/* Synchronization event */
void SGLnotifySync(SglSyncEv ev);

/*
 * Control flow event (unimplemented)
 */
//void SGLnotifyCF(SglCFEv ev);

/* Application complete */
void SGLnotifyFinish();

#ifdef __cplusplus
}
#endif

#endif

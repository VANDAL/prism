#include "ThreadMgmt.h"
#include "FnMgmt.h"

/* \brief These set of functions manage the thread contexts for a program.
 *
 * A function context is a unique path of function calls. A THREAD context
 * is the set of function contexts for a given thread. More simply, a thread
 * context is a representation of the entire call-tree for a given thread. 
 * In a single threaded program, this is just the main thread.
 *
 * Each thread encountered is given its own thread context.
 *
 */

#define _STATIC_ static
#define _STATIC_INLINE_ static inline

void updateWithThreadID(UInt32 tid);

typedef struct _ThreadContext ThreadContext;
struct _ThreadContext{
	FnCxtNode* root;
	FnCxtNode* curr;
	DynamicArray* leaves;
};

_STATIC_INLINE_ void swapThreadContext(UInt32 tid);
_STATIC_INLINE_ void initThreadContextTable();
_STATIC_INLINE_ void initNewThreadContext(UInt32 tid);
_STATIC_INLINE_ void createThreadContext(UInt32 tid);




_STATIC_ DynamicArray* thread_cxt_tbl;

_STATIC_ Bool is_init_t_tbl = false;

_STATIC_ UInt last_tcxt_id = -1;
_STATIC_ UInt curr_tcxt_id = -1;
_STATIC_ UInt num_tcxts = 0;


ThreadContext* curr_tcxt = NULL;

/*********************************************************************/
/*                       External Functions                          */
/*********************************************************************/
/* The first function seen by this manager is expected to have an id
 * of '0'. Every successive function is expected to have an id that is
 * incremented by '1' from the previous id.
 *
 * \param fid is used as an index for a function in the client's program.
 */
void updateWithThreadID(UInt32 tid){
	if ( tid > num_tcxts )
		exitAtError("SGL.updateTID","Invalid thread id");

	if (!is_init_t_tbl)
		initThreadContextTable();

	curr_tcxt_id = tid;

	if ( last_tcxt_id != curr_tcxt_id )
		swapThreadContext(curr_tcxt_id);

	last_tcxt_id = curr_tcxt_id;
}

ThreadContext* getThreadContext(UInt32 tid){
	if (thread_cxt_tbl->arr[tid] == NULL)
		exitAtError("SGL.getThrCxt","Non-existent thread queried");

	return (ThreadContext*)thread_cxt_tbl->arr[tid];
}

/*********************************************************************/
/*                          Implementation                           */
/*********************************************************************/
_STATIC_INLINE_
void swapThreadContext(UInt32 tid){
	if /*uninitialized*/((ThreadContext*)thread_cxt_tbl->arr[tid] == NULL)
		initNewThreadContext(tid);

	curr_tcxt = (ThreadContext*)thread_cxt_tbl->arr[tid];
}

_STATIC_INLINE_
void initNewThreadContext(){
	ThreadContext* new_tcxt;
	new_tcxt = SGL_MALLOC("SGL.ThrCxt",sizeof(*new_tcxt));
	new_tcxt->root = NULL;
	new_tcxt->curr = NULL;

	UInt32 init_leaves = 64;
	new_tcxt->leaves = initDynamicArray(init_leaves);

	appendToDynamicArray((void*)new_tcxt,thread_cxt_tbl);
	num_tcxts++;
}

_STATIC_INLINE_
void initThreadContextTable(){
	if (!is_init_t_tbl){
		UInt32 init_t_cxts = 16;
		thread_cxt_tbl = initDynamicArray(init_t_cxts);
		is_init_t_tbl = true;
	}
}

#include "FnMgmt.h"
#include "SigilData.h"
#include "Vector.h"

#define _STATIC_ static
#define _STATIC_INLINE_ static inline

_STATIC_ Vector* fn_data;
_STATIC_ Vector* fncxt_buf;

_STATIC_ FnCxtNode* curr_fn_cxt;
_STATIC_ Bool is_init_fn_data = false;
_STATIC_ UInt32 num_fns = 0;
_STATIC_ UInt32 num_fncxts = 0;

_STATIC_INLINE_ FnCxtNode* initFnCxt(UInt32 fid);
_STATIC_INLINE_ void initClientFn(UInt32 fid);
_STATIC_INLINE_ void initFnData();
_STATIC_INLINE_ void checkFnID(UInt32 fid);

/****************************************/
/*       Notified of Function Call      */
/****************************************/
void handleFnCall(UInt32 fid, const char* fname)
{
	checkFnID(fid);

	if/*completely new fn*/( fid == num_fns )
	{
		initClientFn(fid);
	}

	/*each call will create a new 'node' 
	  on the call graph except in the
	  case of recursion*/
	curr_fn_cxt = initFnCxt(fid);
}

_STATIC_INLINE_ 
void initClientFn(UInt32 fid)
{
	ClientFnData* new_fn = SGL_MALLOC("SGL.initClientFn",sizeof(*new_fn));
	new_fn->uid = fid;
	new_fn->cxt_node_cnt = 0;

	/*updates vector of fn's
	  indexed directly with fid
	  based on num_fns */
	vector_add(fn_data,new_fn);
	num_fns++;
}

_STATIC_INLINE_
FnCxtNode* initFnCxt(UInt32 fid)
{
	FnCxtNode* cxt = SGL_MALLOC("SGL.initCxtFn",sizeof(*cxt));
	cxt->fid = fid;
	cxt->cedges.caller = curr_fn_cxt;
	cxt->cedges.callees = vector_init(1); //fudge number
	cxt->local_data_cnt = 0;
	cxt->root = SGL_MALLOC("SGL.initCxtFn",sizeof(*(cxt->root)));
	cxt->iops_cnt = 0;
	cxt->flops_cnt = 0;
	return cxt;
}

/****************************************/
/*      Notified of Function Return     */
/****************************************/
void handleFnReturn()
{
	//TODO: check how the first function is handled. Does it get returned from at the main exit?
	FnCxtNode* last_fn = curr_fn_cxt->cedges.caller;
	if ( last_fn == NULL )
	{
		exitAtError("SGL.FnReturn","Cannot return from first function???");
	}

	/* queue the function invocation
	   to write out stats */
	addToOutBuf(curr_fn_cxt);

	/* switch back to our caller */
	curr_fn_cxt = last_fn;
}

/****************************************/
/*           Utility Helpers            */
/****************************************/
ClientFnData* getClientFn(UInt32 fid)
{
	checkFnID(fid);
	return vector_get(fn_data, fid);
}

FnCxtNode* getCurrFnCxt()
{
	return curr_fn_cxt;
}

_STATIC_INLINE_
void addToOutBuf(const FnCxtNode* fncxt)
{
	//TODO !!! if not init
	fncxt_buf = vector_init(500);

	vector_add(fncxt_buf,curr_fn_cxt);
	if ( 500 < vector_used(fncxt_buf) )
	{
		FnCxtNode* fn = vector_get(fncxt_buf, 0);	
		//TODO dump fn stats
		//TODO free all fn data
	}
}

/* The first function seen by this manager is expected to have an id
 * of '0'. Every successive function is expected to have an id that is
 * incremented by '1' from the previous id.
 */
_STATIC_INLINE_
void checkFnID(UInt32 fid)
{
	if/*undefined behavior*/( fid > num_fns )
		exitAtError("SGL.checkFID","Invalid function id");
}

_STATIC_INLINE_ 
void initFnData()
{
	if( !is_init_fn_data )
	{
		curr_fn_cxt = NULL;
		int init_tbl_size = 512;
		fn_data = vector_init(init_tbl_size);
		is_init_fn_data = true;
	}
}

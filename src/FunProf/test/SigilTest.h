#ifndef _SIGIL_TEST_
#define _SIGIL_TEST_

#include "SigilData.h"
#include "Vector.h"
/* Helper functions for testing sigil */

static UInt32 fncxt_cnt = 0;
static Vector* fncxt_vec;
static int is_init_fnvec = 0;

FnCxtNode* spawnFnCxtNode(void)
{
	if (!is_init_fnvec){
		fncxt_vec = vector_init(8);
		is_init_fnvec = 1;
	}

	FnCxtNode* cxtfn = malloc(sizeof(FnCxtNode));
	cxtfn->fid = fncxt_cnt++;

	vector_add(fncxt_vec,(void*)cxtfn);
	return cxtfn;
}

void destroyFnCxtNode(FnCxtNode* cxtfn) { free(cxtfn); }
void destroyAllFnNodes(void) 
{ 
	for (int i=0;i<fncxt_vec->used;++i){
		free(vector_get(fncxt_vec,i));
	}
}

int isFnInList(const FnCxtNode* fn, const FnCxtNodeList* list)
{
	if (!list)
		return 0;

	while(list)
	{
		if (list->fn == fn)
			return 1;
		list = list->next;
	}

	return 0;
}

#endif

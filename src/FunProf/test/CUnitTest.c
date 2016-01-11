#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"
#include "ShadowMemory.c"

//nothing to do
int dummyInit()
{
	return 0;
}

int dummyCleanup() 
{
	return 0;
}

/*********************************************************
 *                SHADOW MEMORY TESTS                    *
 ********************************************************/
void test_InvalidAddr()
{
	//should error out of program
	checkAddrValid(MAX_PRIMARY_ADDRESS+1);
}

void test_ShadowMemInit()
{
	initShadowMemory();

	/* Sanity check, assuming SM
	 * is indexed by lower 19 bits */
	//TODO get rid of
	CU_ASSERT_EQUAL(524288,SM_SIZE);

	int i;
	for(i=0;i<SM_SIZE;++i){
		CU_ASSERT_EQUAL(NULL,DSM.ShadowObject[i].writer);
		CU_ASSERT_EQUAL(NULL,DSM.ShadowObject[i].reader);
	}

	for(i=0;i<PM_SIZE;++i)
		CU_ASSERT(PM[i] == &DSM);
}

void test_getSM()
{
	SecondaryMap* SM;

	unsigned long testaddr1 = 1UL<<23;
	SM = getSMFromAddr(testaddr1);
	CU_ASSERT_NOT_EQUAL(SM,&DSM);
	CU_ASSERT_EQUAL(SM,PM[(testaddr1)>>PM_BITS]);//by definition

	//last valid address
	SM = getSMFromAddr(MAX_PRIMARY_ADDRESS);
	CU_ASSERT_NOT_EQUAL(SM,&DSM);
	CU_ASSERT_EQUAL(SM,PM[MAX_PRIMARY_ADDRESS>>PM_BITS]);
}

void test_getAndSetOneWriter()
{
	Addr testaddr1 = MAX_PRIMARY_ADDRESS;

	FnCxtNode* writer_found;

	//uninitialized
	writer_found = getWriterFromAddr(testaddr1);
	CU_ASSERT_EQUAL(NULL,writer_found);

	//initialize
	FnCxtNode* writer_cxtfn = malloc(sizeof(*writer_cxtfn));
	writer_cxtfn->fid = 0;
	
	setAddrToWriter(testaddr1,writer_cxtfn);
	writer_found = getWriterFromAddr(testaddr1);

	CU_ASSERT_EQUAL(writer_found, writer_cxtfn);
}
/*********************************************************
 *               END SHADOW MEMORY TESTS                 *
 ********************************************************/

int main()
{
	if (CUE_SUCCESS != CU_initialize_registry())
		goto exit;

	CU_pSuite shadow_mem_suite = CU_add_suite("Shadow Memory Tests",dummyInit,dummyCleanup);
	if (NULL == shadow_mem_suite)
		goto exit;

	//CU_ADD_TEST(shadow_mem_suite,test_InvalidAddr);
	if (NULL == CU_ADD_TEST(shadow_mem_suite,test_ShadowMemInit)
	|| NULL == CU_ADD_TEST(shadow_mem_suite,test_getSM)
	|| NULL == CU_ADD_TEST(shadow_mem_suite,test_getAndSetOneWriter))
		goto exit;

	CU_basic_run_tests();

exit:
	CU_cleanup_registry();
	return CU_get_error();
}

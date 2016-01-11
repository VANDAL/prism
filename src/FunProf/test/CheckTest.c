#include "check.h"
#include "FnMgr.c"
#include <stdlib.h>

void setup()
{
	initFnData();
}

void teardown()
{
}

/*********************************************************
 *                FUNCTION MANAGER TESTS                 *
 ********************************************************/

START_TEST(test_init)
{
}
END_TEST


/*********************************************************
 *              END FUNCTION MANAGER TESTS               *
 ********************************************************/

Suite* FnMgrSuite()
{
	Suite* s;
	TCase* tc_init;
	TCase* tc_getAndSet;

	s = suite_create("Function Manager");

	/* init test case */
	tc_init = tcase_create("initialization");

	//tcase_add_test(tc_init, test_init);
	//tcase_add_test(tc_init, test_getSM);
	suite_add_tcase(s,tc_init);

	/* init test case */
	tc_getAndSet = tcase_create("getting/setting writers");

	//tcase_add_checked_fixture(tc, setup, teardown);

	//tcase_add_test(tc_getAndSet, test_ReadAndWriteComb);
	//suite_add_tcase(s,tc_getAndSet);

	tcase_set_timeout(tc_getAndSet, 300);

	return s;
}

int main(void)
{
	int number_failed;
	Suite* s;
	SRunner *sr;

	s = FnMgrSuite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);

	srunner_free(sr);
	return (0 == number_failed) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#include "check.h"
#include "FnComm.c"
#include <time.h>
#include <stdlib.h>


/*********************************************************
 *           Function Communication Tests                *
 ********************************************************/

/* In order to: 
 * As a: 
 * I want to: 
 */

START_TEST(test_one_unique_dep)
{
	/* GIVEN:
	 * A writer has written to an address */

	/* WHEN:
	 * A unique reader reads from the address, a unique read is TODO something */

	/* THEN:
	 * A secondary map is returned, indexed by the upper PM_BITS bits */
}
END_TEST

START_TEST(test_two_unique_dep)
{
	/* GIVEN:
	 * A writer has written to an address */

	/* WHEN:
	 * A secondary map to shadow memory is requested from an address */

	/* THEN:
	 * A secondary map is returned, indexed by the upper PM_BITS bits */
}
END_TEST

START_TEST(test_one_local_dep)
{
	/* GIVEN:
	 * A writer has written to an address */

	/* WHEN:
	 * A secondary map to shadow memory is requested from an address */

	/* THEN:
	 * A secondary map is returned, indexed by the upper PM_BITS bits */
}
END_TEST

START_TEST(test_two_local_dep)
{
	/* GIVEN:
	 * A writer has written to an address */

	/* WHEN:
	 * A secondary map to shadow memory is requested from an address */

	/* THEN:
	 * A secondary map is returned, indexed by the upper PM_BITS bits */
}
END_TEST

/*********************************************************
 *               END SHADOW MEMORY TESTS                 *
 ********************************************************/

Suite* FnCommSuite()
{
	Suite* s;
	TCase* tc_init;
	TCase* tc_setAndget_readers;
	TCase* tc_setAndget_writers;
	TCase* tc_setAndget_mixed;

	s = suite_create("Function Dependency");

	tc_init = tcase_create("Initialization");
	suite_add_tcase(s,tc_init);

	tc_setAndget_writers = tcase_create("Setting/Getting Writers");
	tcase_set_timeout(tc_setAndget_writers, 300);
	suite_add_tcase(s,tc_setAndget_writers);

	tc_setAndget_readers = tcase_create("Setting/Getting Readers");
	suite_add_tcase(s,tc_setAndget_readers);

	tc_setAndget_mixed = tcase_create("Setting/Getting Mixed Reads/Writes");
	suite_add_tcase(s,tc_setAndget_mixed);

	return s;
}

int main(void)
{
	int number_failed;
	Suite* s;
	SRunner *sr;

	s = FnCommSuite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);

	srunner_free(sr);
	return (0 == number_failed) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#include "check.h"
#include "FnShadowMemory.c"
#include <time.h>
#include <stdlib.h>

/*********************************************************
 *                SHADOW MEMORY TESTS                    *
 ********************************************************/

/* In order to: track dependencies between functions
 * As a: interface that has access to reader and writer ids
 * I want to: save and retrieve reader/writer function ids to an address
 */

START_TEST(test_getSM1)
{
	/* GIVEN:
	 * The shadow memory is uninitialized */

	/* WHEN:
	 * A secondary map to shadow memory is requested from an address */
	UInt64 testaddr1 = 1UL<<23;
	SecondaryMap* SM = getSMFromAddr(testaddr1);

	/* THEN:
	 * A secondary map is returned, indexed by the upper PM_BITS bits */
	ck_assert_ptr_ne(SM,&DSM);
	ck_assert_ptr_eq(SM,PM[PM_IDX(testaddr1)]);//by definition

	//when last valid address is requested, a valid SM is returned
	SM = getSMFromAddr(MAX_PRIMARY_ADDRESS);
	ck_assert_ptr_ne(SM,&DSM);
	ck_assert_ptr_eq(SM,PM[PM_IDX(MAX_PRIMARY_ADDRESS)]);

	//TODO when 1 past last valid address is requested, 
	//an error is thrown and the program exits
}
END_TEST

START_TEST(test_getSM2)
{
	Addr test_addr = (1UL<<SM_BITS)-3;
	/* GIVEN:
	 * The shadow memory is uninitialized */
	/* WHEN:
	 * The SM for two addresses in the same SM are queried */
	/* THEN:
	 * The same unique SM is found */
	ck_assert_ptr_ne( getSMFromAddr(test_addr),		&DSM );
	ck_assert_ptr_eq( getSMFromAddr(test_addr+1),getSMFromAddr(test_addr) );
}
END_TEST

START_TEST(test_get_uninit_writer)
{
	/* GIVEN: 
	 * A writer to an address is uninitialized */
	
	/* WHEN:
	 * A writer is requested from that address */
	UInt64 testaddr1 = MAX_PRIMARY_ADDRESS;
	FnCxtUID writer_found;

	/* THEN:
	 * NULL is returned, implying no function wrote to the address */
	writer_found = getWriterFromAddr(testaddr1);
	ck_assert_uint_eq(UNDEF_FN,writer_found);

	/* GIVEN:
	 * The writer to an address is set */
	FnCxtUID writer_id = 100000;
	setWriterToAddrRange(writer_id,testaddr1,1);

	/* WHEN:
	 * The writer is requested from that address */
	writer_found = getWriterFromAddr(testaddr1);

	/* THEN:
	 * The same writer is returned */
	ck_assert_uint_eq(writer_found, writer_id);
}
END_TEST

///////////////////////////////////
// Shadow Memory Writes
///////////////////////////////////

START_TEST(test_setWriterLargeMemorySpace)
{
	/* GIVEN:
	//CAN'T TEST THIS CASE
	 * A writer is set for the entire address space */
	FnCxtUID writer_id = 0;
	//setWriterToAddrRange(writer_cxtfn,0,MAX_PRIMARY_ADDRESS+1);
	setWriterToAddrRange(writer_id,0,(MAX_PRIMARY_ADDRESS+1)/2048);

	/* THEN:
	 * The same writer is returned */
	FnCxtUID writer_found = getWriterFromAddr(10000);
	ck_assert_uint_eq(writer_found, writer_id);
}
END_TEST

START_TEST(test_set_and_get_writer1)
{
	Addr test_addr = (1UL<<SM_BITS)-3;

	/* GIVEN:
	 * A writer1 is set to addresses that span two SMs */
	FnCxtUID writer_id = (1<<25)+1;
	setWriterToAddrRange(writer_id, test_addr,		5);

	/* WHEN:
	 * The SMs are retrieved for an address written to by writer1 */
	/* WHEN:
	 * The target address is on the boundary of a new SM */
	/* THEN:
	 * A unique SM is found, different than the beginning address' SM */
	ck_assert_ptr_ne( getSMFromAddr(test_addr+3),	&DSM );
	ck_assert_ptr_ne( getSMFromAddr(test_addr+3),	getSMFromAddr(test_addr) );
}
END_TEST

START_TEST(test_set_and_get_writer2)
{
	Addr test_addr = (1UL<<SM_BITS)-3;

	/* GIVEN:
	 * A writer1 is set to addresses that span two SMs */
	FnCxtUID writer_id1 = 1<<22;
	setWriterToAddrRange(writer_id1, test_addr,		5);

	/* GIVEN:
	 * A writer2 is set for addresses partially overlapping with writer1 */
	FnCxtUID writer_id2 = 0;
	setWriterToAddrRange(writer_id2, test_addr+5,	5);

	/* GIVEN:
	 * A writer3 is set for addresses partially overlapping with writer2 */
	FnCxtUID writer_id3 = 10;
	setWriterToAddrRange(writer_id3, test_addr+10,	5);

	/* WHEN:
	 * A list of writer functions is retrieved for 
	 * 2 addresses spanned by one writer */
	/* THEN:
	 * The one writer is returned */
	for (UInt64 i=test_addr; i<test_addr+3; ++i)
	{
		ck_assert_uint_eq( getWriterFromAddr(i), writer_id1 );
	}
}
END_TEST

START_TEST(test_set_and_get_writer3)
{
	Addr test_addr = (1UL<<SM_BITS)-3;

	/* GIVEN:
	 * A writer1 is set to addresses that span two SMs */
	FnCxtUID writer_id1 = (1<<30)-3;
	setWriterToAddrRange(writer_id1, test_addr,		5);

	/* GIVEN:
	 * A writer2 is set for addresses partially overlapping with writer1 */
	FnCxtUID writer_id2 = 100;
	setWriterToAddrRange(writer_id2, test_addr+5,	5);

	/* GIVEN:
	 * A writer3 is set for addresses partially overlapping with writer2 */
	FnCxtUID writer_id3 = 0;
	setWriterToAddrRange(writer_id3, test_addr+10,	5);





	//FnCxtNodeList* fn_list_1, *fn_list_2, *fn_list_3; 
	///* WHEN:
	// * A list of writer functions is retrieved for 
	// * 2 addresses spanned by two writers(1&2) */
	//fn_list_1 = getWritersFromAddrRange(test_addr, 2);
	//fn_list_2 = getWritersFromAddrRange(test_addr, 5);
	///* THEN:
	// * Writer1 and writer2 are both returned as the writers */
	//ck_assert_uint_eq( getSizeOfList(fn_list_2), 1 );
	//ck_assert_ptr_eq( fn_list_1->fn, fn_list_2->fn );
	//ck_assert( isFnInList(writer_cxtfn_1,fn_list_1) );
	//ck_assert( isFnInList(writer_cxtfn_1,fn_list_2) );

	//fn_list_3 = getWritersFromAddrRange(test_addr, 11);
	//fn_list_2 = getWritersFromAddrRange(test_addr+5, 5);
	//ck_assert_uint_eq( getSizeOfList(fn_list_3), 3 );
	//ck_assert( isFnInList(writer_cxtfn_1,fn_list_3) );
	//ck_assert( isFnInList(writer_cxtfn_2,fn_list_3) );
}
END_TEST

///////////////////////////////////
// Shadow Memory Reads
///////////////////////////////////
START_TEST(test_set_and_set_reader1)
{
	Addr testaddr1 = MAX_PRIMARY_ADDRESS-6;

	/* GIVEN: No readers have been initialized */
	/* THEN: No readers have been initialized */
	ck_assert( isReaderAtAddr(UNDEF_FN, testaddr1) );

	/* GIVEN: One reader has been set */
	setReaderToAddrRange( 0, testaddr1, 1);
	/* THEN: That reader is the only reader function found in the address range */
	ck_assert( isReaderAtAddr(0, testaddr1) );
}
END_TEST

START_TEST(test_set_and_set_reader2)
{
	Addr testaddr1 = MAX_PRIMARY_ADDRESS-6;

	/* GIVEN: Five readers has been set */
	FnCxtUID reader_id1 = 0;
	FnCxtUID reader_id2 = 1;
	FnCxtUID reader_id3 = 100;
	FnCxtUID reader_id4 = (1<<31)+2;
	FnCxtUID reader_id5 = (1<<23)-1;

	setReaderToAddrRange(reader_id1,testaddr1,1);
	setReaderToAddrRange(reader_id2,testaddr1,1);
	setReaderToAddrRange(reader_id3,testaddr1,1);
	setReaderToAddrRange(reader_id4,testaddr1+1,2);
	setReaderToAddrRange(reader_id5,testaddr1,3);

	/* THEN: Those readers are found in the address range */
//	readers_found = getReadersFromAddrRange(testaddr1,4);
//	ck_assert_uint_eq( getSizeOfList(readers_found), 5 );
//
//	readers_found = getReadersFromAddrRange(testaddr1,1);
//	ck_assert_uint_eq( getSizeOfList(readers_found), 4 );
//	ck_assert( isFnInList(reader_cxtfn1, readers_found) );
//	ck_assert( isFnInList(reader_cxtfn2, readers_found) );
//	ck_assert( isFnInList(reader_cxtfn3, readers_found) );
//	ck_assert( isFnInList(reader_cxtfn5, readers_found) );
//	ck_assert(! isFnInList(reader_cxtfn4, readers_found) );
//
//	readers_found = getReadersFromAddrRange(testaddr1+2,1);
//	ck_assert_uint_eq( getSizeOfList(readers_found), 2 );
//	ck_assert( isFnInList(reader_cxtfn5, readers_found) );
//	ck_assert( isFnInList(reader_cxtfn4, readers_found) );
//	ck_assert(! isFnInList(reader_cxtfn3, readers_found) );
//	ck_assert(! isFnInList(reader_cxtfn2, readers_found) );
//	ck_assert(! isFnInList(reader_cxtfn1, readers_found) );
}
END_TEST

START_TEST(test_set_and_get_mixed1)
{
	UInt64 testaddr = 12345;
	FnCxtUID fncxtid1 = (1<<27)-1;
	FnCxtUID fncxtid2 = (1UL<<32)-2;
	FnCxtUID fncxtid3 = 13;
	FnCxtUID fncxtid4 = 0;
	FnCxtUID fncxtid5 = 1;

	/* GIVEN: Five readers have been set */
	setReaderToAddrRange(fncxtid1,testaddr,1);
	setReaderToAddrRange(fncxtid2,testaddr,1);
	setReaderToAddrRange(fncxtid3,testaddr,3);
	setReaderToAddrRange(fncxtid4,testaddr+1,10);
	setReaderToAddrRange(fncxtid5,testaddr-5,10);
	ck_assert( !isReaderAtAddr(UNDEF_FN, testaddr) );
	ck_assert( !isReaderAtAddr(UNDEF_FN, testaddr-3) );
	ck_assert( !isReaderAtAddr(UNDEF_FN, testaddr+4) );

	/* WHEN: writers write to the read addresses (confusing I know) */
	/* THEN: the read-from shadow object will be cleared */
	setWriterToAddrRange(fncxtid5,testaddr,1);
	setWriterToAddrRange(fncxtid3,testaddr,1);
	setWriterToAddrRange(fncxtid4,testaddr,3);
	setWriterToAddrRange(fncxtid2,testaddr+1,10);
	setWriterToAddrRange(fncxtid1,testaddr-5,10);
	ck_assert( isReaderAtAddr(UNDEF_FN, testaddr) );
	ck_assert( isReaderAtAddr(UNDEF_FN, testaddr-3) );
	ck_assert( isReaderAtAddr(UNDEF_FN, testaddr+4) );
}
END_TEST

START_TEST(test_set_and_get_mixed2)
{
	srand(time(NULL));

	/* GIVEN: Five writers have been set */
	UInt64 testaddr = 123450;
	FnCxtUID fncxtid1 = rand() % (UNDEF_FN-1);
	FnCxtUID fncxtid2 = rand() % (UNDEF_FN-1);
	FnCxtUID fncxtid3 = rand() % (UNDEF_FN-1);
	FnCxtUID fncxtid4 = rand() % (UNDEF_FN-1);
	FnCxtUID fncxtid5 = rand() % (UNDEF_FN-1);
	setWriterToAddrRange(fncxtid1,testaddr-20,100);
	setWriterToAddrRange(fncxtid2,testaddr+100,15);
	setWriterToAddrRange(fncxtid3,testaddr-300,3);
	setWriterToAddrRange(fncxtid4,testaddr+10000,10);
	setWriterToAddrRange(fncxtid5,testaddr-50,10);

	/* WHEN: readers read from the write addresses */
	/* THEN: the readers and writers will be preserved */
	setReaderToAddrRange(fncxtid1,testaddr-300,320);
	setReaderToAddrRange(fncxtid2,testaddr-200,500);
	setReaderToAddrRange(fncxtid3,testaddr,20000);

	UInt64 testaddr2 = testaddr-100;
	for (UInt64 i=testaddr2; i<testaddr+50; ++i)
	{
		ck_assert(  isReaderAtAddr(fncxtid2, i) );
		ck_assert(! isReaderAtAddr(fncxtid4, i) );
		ck_assert(! isReaderAtAddr(fncxtid5, i) );

		if (i < testaddr+20)
			ck_assert(  isReaderAtAddr(fncxtid1, i) );
		else
			ck_assert(! isReaderAtAddr(fncxtid1, i) );

		if (i < testaddr)
			ck_assert(! isReaderAtAddr(fncxtid3, i) );
		else
			ck_assert(  isReaderAtAddr(fncxtid3, i) );
	}

//	readers_found = getReadersFromAddrRange(testaddr+100,10000);
//	ck_assert(! isFnInList(cxtfn1, readers_found) );
//	ck_assert( isFnInList(cxtfn2, readers_found) );
//	ck_assert( isFnInList(cxtfn3, readers_found) );
//	ck_assert(! isFnInList(cxtfn4, readers_found) );
//	ck_assert(! isFnInList(cxtfn5, readers_found) );
//
//	FnCxtNodeList* writers_found;
//	writers_found = getWritersFromAddrRange(testaddr+9000, 1);
//	ck_assert_uint_eq( getSizeOfList(writers_found), 0 );
//
//	writers_found = getWritersFromAddrRange(testaddr+10000, 15);
//	ck_assert_uint_eq( getSizeOfList(writers_found), 1 );
//	ck_assert( isFnInList(cxtfn4, writers_found) );
}
END_TEST



/*********************************************************
 *               END SHADOW MEMORY TESTS                 *
 ********************************************************/

Suite* ShadowMemorySuite()
{
	Suite* s;
	TCase* tc_init;
	TCase* tc_setAndget_readers;
	TCase* tc_setAndget_writers;
	TCase* tc_setAndget_mixed;

	s = suite_create("Shadow Memory");

	tc_init = tcase_create("Initialization");
	tcase_add_test(tc_init, test_getSM1);
	tcase_add_test(tc_init, test_getSM2);
	tcase_add_test(tc_init, test_get_uninit_writer);
	suite_add_tcase(s,tc_init);

	tc_setAndget_writers = tcase_create("Setting/Getting Writers");
	tcase_add_test(tc_setAndget_writers, test_setWriterLargeMemorySpace);
	tcase_add_test(tc_setAndget_writers, test_set_and_get_writer1);
	tcase_add_test(tc_setAndget_writers, test_set_and_get_writer2);
	tcase_add_test(tc_setAndget_writers, test_set_and_get_writer3);
	tcase_set_timeout(tc_setAndget_writers, 300);
	suite_add_tcase(s,tc_setAndget_writers);

	tc_setAndget_readers = tcase_create("Setting/Getting Readers");
	tcase_add_test(tc_setAndget_readers, test_set_and_set_reader1);
	tcase_add_test(tc_setAndget_readers, test_set_and_set_reader2);
	suite_add_tcase(s,tc_setAndget_readers);

	tc_setAndget_mixed = tcase_create("Setting/Getting Mixed Reads/Writes");
	tcase_add_test(tc_setAndget_mixed, test_set_and_get_mixed1);
	tcase_add_test(tc_setAndget_mixed, test_set_and_get_mixed2);
	suite_add_tcase(s,tc_setAndget_mixed);

	return s;
}

int main(void)
{
	int number_failed;
	Suite* s;
	SRunner *sr;

	s = ShadowMemorySuite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);

	srunner_free(sr);
	return (0 == number_failed) ? EXIT_SUCCESS : EXIT_FAILURE;
}

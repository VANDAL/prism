#include "check.h"
#include "ReaderMemPool.c"
#include <stdlib.h>
#include <time.h>

/*********************************************************
 *               Reader Memory Pool Tests                *
 ********************************************************/

/* In order to: 
 * As a: 
 * I want to: 
 */

START_TEST(test_init)
{
	ReaderPool pool;
	readerPoolInit(&pool);
	ck_assert_uint_eq(pool.num_initialized, 0);
	ck_assert_uint_eq(pool.num_free_blocks, SM_SIZE);

	UInt32 *indx = malloc(SM_SIZE*sizeof(UInt32));
	for ( UInt32 i=0; i<SM_SIZE; ++i)
	{
		indx[i] = readerPoolAlloc(&pool);	
	}

	ck_assert_uint_eq(pool.num_initialized, SM_SIZE);
	ck_assert_uint_eq(pool.num_free_blocks, 0);

	free(pool.pool);
}
END_TEST

START_TEST(test_realloc)
{
	ReaderPool pool;
	readerPoolInit(&pool);
	UInt32 *indx = malloc(SM_SIZE*sizeof(UInt32));

	for ( UInt32 j=0; j<100; ++j ) 
	{
		for ( UInt32 i=0; i<SM_SIZE; ++i )
		{
			indx[i] = readerPoolAlloc(&pool);
		}
	}

	time_t t;
	srand((UInt32) time(&t));

	ck_assert_uint_eq( pool.pool[(rand()%SM_SIZE)].reader,			UNDEF_READER );
	ck_assert_uint_eq( pool.pool[(rand()%SM_SIZE)].next_reader_idx,	UNDEF_READER_IDX);

	ck_assert_uint_eq( pool.pool[SM_SIZE].reader,			UNDEF_READER );
	ck_assert_uint_eq( pool.pool[SM_SIZE].next_reader_idx,	UNDEF_READER_IDX);

	ck_assert_uint_eq( pool.pool[3*SM_SIZE+(rand()%SM_SIZE)].reader,				UNDEF_READER );
	ck_assert_uint_eq( pool.pool[3*SM_SIZE+(rand()%SM_SIZE)].next_reader_idx,	UNDEF_READER_IDX );

	ck_assert_uint_eq( pool.pool[99*SM_SIZE].reader,			UNDEF_READER );
	ck_assert_uint_eq( pool.pool[99*SM_SIZE].next_reader_idx,	UNDEF_READER_IDX );

	free(pool.pool);
}
END_TEST

START_TEST(test_set_one_sm)
{
	ReaderPool pool;
	readerPoolInit(&pool);
	UInt32 *indx = malloc(SM_SIZE*sizeof(UInt32));
	UInt32 *vals = malloc(SM_SIZE*sizeof(UInt32));

	time_t t;
	srand((UInt32) time(&t));

	for ( UInt32 i=0; i<SM_SIZE; ++i )
	{
		indx[i] = readerPoolAlloc(&pool);
	}

	for ( UInt32 i=0; i<SM_SIZE; ++i ) 
	{
		vals[i] = rand() % RAND_MAX;
		readerPoolSetReader(&pool, vals[i], indx[i]);
	}

	for ( UInt32 i=0; i<SM_SIZE; ++i )
	{
		ck_assert_uint_eq(vals[i], readerPoolGetReader(&pool,indx[i]));
	}

	ck_assert_uint_eq(SM_SIZE,pool.total_blocks);

	free(pool.pool);
}
END_TEST


START_TEST(test_set_one_plus_sm)
{
	ReaderPool pool;
	readerPoolInit(&pool);
	const UInt64 buf_size = SM_SIZE+20;
	UInt32 *indx = malloc(buf_size*sizeof(UInt32));
	UInt32 *vals = malloc(buf_size*sizeof(UInt32));

	time_t t;
	srand((UInt32) time(&t));

	for ( UInt32 i=0; i<buf_size; ++i )
	{
		indx[i] = readerPoolAlloc(&pool);
	}

	for ( UInt32 i=0; i<buf_size; ++i ) 
	{
		vals[i] = rand() % RAND_MAX;
		readerPoolSetReader(&pool, vals[i], indx[i]);
	}

	for ( UInt32 i=0; i<buf_size; ++i )
	{
		ck_assert_uint_eq(vals[i], readerPoolGetReader(&pool,indx[i]));
	}

	ck_assert_uint_eq(2*SM_SIZE,pool.total_blocks);

	free(pool.pool);
}
END_TEST

//ck_asserts exploding a tmpfile() buffer
START_TEST(test_set_big)
{
	ReaderPool pool;
	readerPoolInit(&pool);
	const UInt64 buf_size = 128*SM_SIZE;
	UInt32 *indx = malloc(buf_size*sizeof(UInt32));
	UInt32 *vals = malloc(buf_size*sizeof(UInt32));

	time_t t;
	srand((UInt32) time(&t));

	for ( UInt32 i=0; i<buf_size; ++i )
	{
		indx[i] = readerPoolAlloc(&pool);
	}

	for ( UInt32 i=0; i<buf_size; ++i ) 
	{
		vals[i] = rand() % RAND_MAX;
		readerPoolSetReader(&pool, vals[i], indx[i]);
	}

	for ( UInt32 i=0; i<buf_size; ++i )
	{
		ck_assert_uint_eq(vals[i], readerPoolGetReader(&pool,indx[i]));
	}

	free(pool.pool);
}
END_TEST

START_TEST(test_dealloc_alloc)
{
	ReaderPool pool;
	readerPoolInit(&pool);
	const UInt64 buf_size = 3*SM_SIZE;
	UInt32 *indx = malloc(buf_size*sizeof(UInt32));
	UInt32 *vals = malloc(buf_size*sizeof(UInt32));
	UInt32 *stack = malloc((buf_size/8)*sizeof(UInt32));

	time_t t;
	srand((UInt32) time(&t));

	for ( UInt32 i=0; i<buf_size; ++i )
	{
		indx[i] = readerPoolAlloc(&pool);	
	}

	for ( UInt32 i=0; i<buf_size; ++i ) 
	{
		vals[i] = rand() % RAND_MAX;
		readerPoolSetReader(&pool, vals[i], indx[i]);
	}

	/* not much to test here, minimal implementation */
	for ( UInt32 i=0; i<buf_size; i+=8 )
	{
		readerPoolFree(&pool, indx[i]);	
		stack[i/8] = pool.head;
	}

	for ( UInt32 i=0; i<buf_size; i+=8 )
	{
		ck_assert_uint_eq(readerPoolAlloc(&pool),stack[buf_size/8-i/8-1]);
	}

	free(pool.pool);
}
END_TEST

Suite* ReaderMemPoolSuite()
{
	Suite* s;
	TCase* tc_pool_mem;
	TCase* tc_pool_setget;

	s = suite_create("Reader Memory Pool");

	tc_pool_mem = tcase_create("MemPool Alloc/Frees");
	tcase_set_timeout(tc_pool_mem, 30);
	tcase_add_test(tc_pool_mem, test_init);
	tcase_add_test(tc_pool_mem, test_realloc);
	tcase_add_test(tc_pool_mem, test_dealloc_alloc);


	tc_pool_setget = tcase_create("MemPool setting/getting");
	tcase_add_test(tc_pool_setget, test_set_one_sm);
	tcase_add_test(tc_pool_setget, test_set_one_plus_sm);

	suite_add_tcase(s,tc_pool_mem);
	suite_add_tcase(s,tc_pool_setget);

	return s;
}

int main(void)
{
	int number_failed;
	Suite* s;
	SRunner *sr;

	s = ReaderMemPoolSuite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);

	srunner_free(sr);
	return (0 == number_failed) ? EXIT_SUCCESS : EXIT_FAILURE;
}

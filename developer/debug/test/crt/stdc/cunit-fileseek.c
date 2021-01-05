/*
    Copyright © 2021, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <stdlib.h>
#include <assert.h>

#include <CUnit/Basic.h>

/* handles for the respective tests */
static FILE *fd = NULL;
static BPTR file = BNULL;

/* storage used during testing */
static char buffer[32];
static int i;

/* The suite initialization function.
  * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    FILE *tmpfd = fopen( "T:seek.txt", "wb" );
    if ( !tmpfd )
    {
        return 1;
    }
    fprintf( tmpfd, "() does not work!\n" );
    fclose(tmpfd);

    return 0;
}

/* The suite cleanup function.
  * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
    if (file)
        Close(file);
    if (fd)
        fclose(fd);
    return 0;
}

/* Simple test of fopen().
 */
void testFOPEN(void)
{
    fd = fopen( "T:seek.txt", "rb" );
    CU_ASSERT( fd != NULL );
}

/* Simple test of ConvertPixels(argb -> rgb15).
 */
void testFSEEK(void)
{

  i = fread( buffer, 1, 1, fd );
  CU_ASSERT( i == 1 );

  i += fread( &buffer[1], 1, 6, fd );
  CU_ASSERT( i == 7 );

  fseek( fd, 4, SEEK_CUR );
  i = fread( &buffer[7], 1, 11, fd );
  buffer[7+i]=0;
  printf( "fseek%s", buffer );
  fclose(fd);
  fd = NULL;
}

/* Simple test of Open().
 */
void testOPEN(void)
{
    file = Open( "T:seek.txt", MODE_OLDFILE );
    CU_ASSERT( file != NULL );
}

/* Simple test of Seek().
 */
void testSEEK(void)
{
    /* Seek() */
    i = Read( file, buffer, 7 );
    CU_ASSERT(7 == i);
    Seek( file, 4, OFFSET_CURRENT );
    i += Read( file, &buffer[7], 11 );
    CU_ASSERT(18 == i);
    Close(file);
    file = BNULL;
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

   /* add a suite to the registry */
    pSuite = CU_add_suite("FileSeek_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

   /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "test of fopen()", testFOPEN)) ||
        (NULL == CU_add_test(pSuite, "test of fseek()", testFSEEK)) ||
        (NULL == CU_add_test(pSuite, "test of Open()", testOPEN)) ||
        (NULL == CU_add_test(pSuite, "test of Seek()", testSEEK)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

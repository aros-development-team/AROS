/*
    Copyright (C) 2021, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>
#include <dos/dos.h>
#include <stdlib.h>
#include <stdio.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

/* handle for the respective tests */
static FILE *fd = NULL;

/* storage used during testing */
static char buffer[32];
static int i;

/* The suite initialization function.
  * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    return 0;
}

/* The suite cleanup function.
  * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
    if (fd)
        if (0 != fclose(fd))
            return -1;
    return 0;
}

/* Simple test of fopen(file,"wb").
 */
void testFOPENW(void)
{
    fd = fopen( "T:cunit-crt-fileseek.txt", "wb" );
    if (fd)
    {
        CU_PASS("");
    }
    else
    {
        CU_FAIL("NULL == (fd = fopen( \"T:cunit-crt-fileseek.txt\", \"wb\" ))");
    }
}

/* Simple test of fprintf(file,"wb").
 */
void testFPRINTF(void)
{
    CU_SKIP_IF(fd == NULL);
    if (fd)
    {
        CU_ASSERT(18 == fprintf( fd, "%s", "() does not work!\n" ));
    }
}

/* Simple test of fclose().
 */
void testFCLOSE(void)
{
    CU_SKIP_IF(fd == NULL);
    if (fd)
    {
        CU_ASSERT(0 == fclose(fd));
        fd = NULL;
    }
}

/* Simple test of fopen(file,"rb").
 */
void testFOPENR(void)
{
    fd = fopen( "T:cunit-crt-fileseek.txt", "rb" );
    if (fd)
    {
        CU_PASS("");
    }
    else
    {
        CU_FAIL("NULL == (fd = fopen( \"T:cunit-crt-fileseek.txt\", \"rb\" ))");
    }
}

/* Simple test of fread().
 */
void testFREAD(void)
{
    CU_SKIP_IF(fd == NULL);
    if (fd)
    {
        CU_ASSERT(1 == (i = fread( buffer, 1, 1, fd )));
        CU_ASSERT(7 == (i += fread( &buffer[1], 1, 6, fd )));
    }
}

/* Simple test of fseek().
 */
void testFSEEK(void)
{
    CU_SKIP_IF(fd == NULL);
    if (fd)
    {
        CU_ASSERT(0 == fseek( fd, 4, SEEK_CUR ));
        CU_ASSERT(7 == (i = fread( &buffer[7], 1, 11, fd )));
        CU_ASSERT(0 == fseek(fd, 0, SEEK_SET));
        CU_ASSERT(11 == (i = fread( &buffer[7], 1, 11, fd )));
    }
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
    if (
        (NULL == CU_add_test(pSuite, "test of fopen(\"T:cunit-crt-fileseek.txt\",'wb')", testFOPENW)) ||
        (NULL == CU_add_test(pSuite, "test of fprintf()", testFPRINTF)) ||
        (NULL == CU_add_test(pSuite, "test of fclose()", testFCLOSE)) ||
        (NULL == CU_add_test(pSuite, "test of fopen(\"T:cunit-crt-fileseek.txt\",'rb')", testFOPENR)) ||
        (NULL == CU_add_test(pSuite, "test of fread()", testFREAD)) ||
        (NULL == CU_add_test(pSuite, "test of fseek()", testFSEEK)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-FileSeek");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

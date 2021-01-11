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
#include <CUnit/Automated.h>

/* handles for the respective tests */
static BPTR file = BNULL;

/* storage used during testing */
static char buffer[32];

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
    if (file)
        if (0 ==Close (file))
            return -1;
    return 0;
}

/* Simple test of Open().
 */
void testOPENW(void)
{
    CU_ASSERT(NULL != (file = Open( "T:cunit-dos-fileseek.txt", MODE_NEWFILE )));
}

/* Simple test of Write().
 */
void testWRITE(void)
{
    CU_SKIP_IF(file == NULL);
    if (file)
    {
        CU_ASSERT(0 != Write(file,"() does not work!\n",18));
    }
}

/* Simple test of Close().
 */
void testCLOSE(void)
{
    CU_SKIP_IF(file == NULL);
    if (file)
    {
        CU_ASSERT(0 != Close(file));
        file = BNULL;
    }
}

/* Simple test of Open().
 */
void testOPENR(void)
{
    CU_ASSERT(NULL != (file = Open( "T:cunit-dos-fileseek.txt", MODE_OLDFILE)));
}

/* Simple test of Read().
 */
void testREAD(void)
{
    CU_SKIP_IF(file == NULL);
    if (file)
    {
        CU_ASSERT(7 == Read( file, buffer, 7 ));
    }
}

/* Simple test of Seek().
 */
void testSEEK(void)
{
    CU_SKIP_IF(file == NULL);
    if (file)
    {
        /* Seek() */
        CU_ASSERT(-1 != Seek( file, 4, OFFSET_CURRENT ));
        CU_ASSERT(-1 != Read( file, &buffer[7], 11 ));
        CU_ASSERT(-1 != Seek( file, 4, OFFSET_BEGINNING ));
        CU_ASSERT(11 == Read( file, &buffer[18], 11 ));
        CU_ASSERT(-1 != Seek( file, 0, OFFSET_END ));
        CU_ASSERT(0 == Read( file, &buffer[29], 1 ));
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
    if ((NULL == CU_add_test(pSuite, "test of Open(\"T:cunit-dos-fileseek.txt\",MODE_NEWFILE)", testOPENW)) ||
        (NULL == CU_add_test(pSuite, "test of Write()", testWRITE)) ||
        (NULL == CU_add_test(pSuite, "test of Close()", testCLOSE)) ||
        (NULL == CU_add_test(pSuite, "test of Open(\"T:cunit-dos-fileseek.txt\",MODE_OLDFILE)", testOPENR)) ||
        (NULL == CU_add_test(pSuite, "test of Read()", testREAD)) ||
        (NULL == CU_add_test(pSuite, "test of Seek()", testSEEK)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("DOSUnitTests");
    CU_set_output_filename("DOS-FileSeek");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

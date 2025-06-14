/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>
#include <dos/dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

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
    return 0;
}

/* Simple test of realpath() with an absolute path.
 */
void test_realpath_basic(void)
{
    char resolved[PATH_MAX];
    char *result;

    result = realpath("/tmp", resolved);

    CU_ASSERT(result != NULL);
    CU_ASSERT(strcmp(resolved, "/tmp") == 0);
}

/* Simple test of realpath() resolving a symlink.
 */
void test_realpath_symlink(void)
{
    char resolved[PATH_MAX];
    char *result;

    CU_SKIP_IF(TRUE);
#warning: TODO: create a symlink then resolve the target..

    CU_ASSERT(result != NULL);
    CU_ASSERT(strcmp(resolved, "/tmp") == 0);
}

/* Simple test of realpath() with a relative path.
 */
void test_realpath_dotdot(void)
{
    char resolved[PATH_MAX];
    char *result;

    result = realpath("/tmp/../tmp", resolved);

    CU_ASSERT(result != NULL);
    CU_ASSERT(strcmp(resolved, "/tmp") == 0);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

   /* add a suite to the registry */
    pSuite = CU_add_suite("PosixC_realpath_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

   /* add the tests to the suite */
    if (
        (NULL == CU_add_test(pSuite, "test of posix realpath(\"/tmp\")", test_realpath_basic)) ||
        (NULL == CU_add_test(pSuite, "test of posix realpath(\"/tmp/<symlink>\")", test_realpath_symlink)) ||
        (NULL == CU_add_test(pSuite, "test of posix realpath(\"/tmp/../tmp\")", test_realpath_dotdot)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-PosixC_realpath");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

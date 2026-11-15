/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/utility.h>
#include <string.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#include <aros/debug.h>

/* storage used during testing */
static char buffer[100];


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

/* Test of CheckDate() with invalid date.
 */
void testCHECKDATEINVALID(void)
{
    struct ClockData cd =
    {
        .sec = 0,
        .min = 0,
        .hour = 0,
        .mday = 29,
        .month = 2,
        .year = 2001,
        .wday = 0
	};

    /*
     * Must return 0 for 29 February in non-leap years.
     */
    CU_ASSERT(0 == CheckDate(&cd));
}

/* Test of CheckDate() with valid date.
 */
void testCHECKDATEVALID(void)
{
    struct ClockData cd =
    {
        .sec = 0,
        .min = 0,
        .hour = 0,
        .mday = 29,
        .month = 2,
        .year = 2004,
        .wday = 0
	};

    /*
     * Must not return 0 for 29 February in leap years.
     */
    CU_ASSERT(0 != CheckDate(&cd));
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("CheckDate_Suite", init_suite, clean_suite);
    if (NULL == pSuite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "test with invalid date", testCHECKDATEINVALID)) ||
        (NULL == CU_add_test(pSuite, "test with valid date", testCHECKDATEVALID)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("UtilityUnitTests");
    CU_set_output_filename("Utility-CheckDate");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

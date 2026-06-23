/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the C99 (ISO/IEC 9899:1999) integer functions added to
    <stdlib.h> and <inttypes.h>: atoll(), llabs(), lldiv(), imaxabs() and
    imaxdiv().
*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

int init_suite(void)
{
    return 0;
}

int clean_suite(void)
{
    return 0;
}

void testATOLL(void)
{
    CU_ASSERT_EQUAL(atoll("9000000000"), 9000000000LL);
    CU_ASSERT_EQUAL(atoll("-9000000000"), -9000000000LL);
    CU_ASSERT_EQUAL(atoll("  42abc"), 42LL);
}

void testLLABS(void)
{
    CU_ASSERT_EQUAL(llabs(9000000000LL), 9000000000LL);
    CU_ASSERT_EQUAL(llabs(-9000000000LL), 9000000000LL);
    CU_ASSERT_EQUAL(llabs(0LL), 0LL);
}

void testLLDIV(void)
{
    lldiv_t d = lldiv(9000000000LL, 7LL);
    CU_ASSERT_EQUAL(d.quot * 7LL + d.rem, 9000000000LL);

    d = lldiv(-17LL, 5LL);
    CU_ASSERT_EQUAL(d.quot, -3LL);    /* truncation toward zero */
    CU_ASSERT_EQUAL(d.rem, -2LL);
}

void testIMAXABS(void)
{
    CU_ASSERT_EQUAL(imaxabs((intmax_t)-12345), (intmax_t)12345);
    CU_ASSERT_EQUAL(imaxabs((intmax_t)12345), (intmax_t)12345);
}

void testIMAXDIV(void)
{
    imaxdiv_t d = imaxdiv((intmax_t)100, (intmax_t)7);
    CU_ASSERT_EQUAL(d.quot, (intmax_t)14);
    CU_ASSERT_EQUAL(d.rem, (intmax_t)2);
    CU_ASSERT_EQUAL(d.quot * 7 + d.rem, (intmax_t)100);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("stdlib_C99_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "atoll()", testATOLL)) ||
        (NULL == CU_add_test(pSuite, "llabs()", testLLABS)) ||
        (NULL == CU_add_test(pSuite, "lldiv()", testLLDIV)) ||
        (NULL == CU_add_test(pSuite, "imaxabs()", testIMAXABS)) ||
        (NULL == CU_add_test(pSuite, "imaxdiv()", testIMAXDIV)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-Stdlib-C99");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

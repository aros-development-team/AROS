/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the C99 (ISO/IEC 9899:1999) string-to-number conversion
    functions: strtoll(), strtoull(), strtof(), strtold() and the
    <inttypes.h> strtoimax() / strtoumax().
*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

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

/* ---- strtoll() / strtoull() ----------------------------------------- */

void testSTRTOLL_basic(void)
{
    char *end;

    CU_ASSERT_EQUAL(strtoll("123", &end, 10), 123LL);
    CU_ASSERT_EQUAL(strtoll("-9000000000", &end, 10), -9000000000LL);
    CU_ASSERT_EQUAL(strtoll("0x7fffffffffffffff", &end, 0), 0x7fffffffffffffffLL);

    const char *s = "12x";
    CU_ASSERT_EQUAL(strtoll(s, &end, 10), 12LL);
    CU_ASSERT_PTR_EQUAL(end, s + 2);
}

void testSTRTOLL_noconv(void)
{
    char *end;
    const char *s = "+";
    CU_ASSERT_EQUAL(strtoll(s, &end, 10), 0LL);
    CU_ASSERT_PTR_EQUAL(end, s);
}

void testSTRTOLL_overflow(void)
{
    char *end;

    errno = 0;
    CU_ASSERT_EQUAL(strtoll("999999999999999999999999", &end, 10), LLONG_MAX);
    CU_ASSERT_EQUAL(errno, ERANGE);

    errno = 0;
    CU_ASSERT_EQUAL(strtoll("-999999999999999999999999", &end, 10), LLONG_MIN);
    CU_ASSERT_EQUAL(errno, ERANGE);
}

void testSTRTOULL_basic(void)
{
    char *end;

    CU_ASSERT_EQUAL(strtoull("18446744073709551615", &end, 10), ULLONG_MAX);
    CU_ASSERT_EQUAL(strtoull("ffffffffffffffff", &end, 16), ULLONG_MAX);
    CU_ASSERT_EQUAL(strtoull("-1", &end, 10), ULLONG_MAX);
}

void testSTRTOULL_overflow(void)
{
    char *end;

    errno = 0;
    CU_ASSERT_EQUAL(strtoull("99999999999999999999999999", &end, 10), ULLONG_MAX);
    CU_ASSERT_EQUAL(errno, ERANGE);
}

/* ---- strtof() / strtold() ------------------------------------------- */

void testSTRTOF(void)
{
    char *end;
    float f = strtof("3.5", &end);
    CU_ASSERT_TRUE(f > 3.4f && f < 3.6f);
    CU_ASSERT_EQUAL(*end, '\0');

    const char *s = "2.0zz";
    f = strtof(s, &end);
    CU_ASSERT_TRUE(f > 1.9f && f < 2.1f);
    CU_ASSERT_PTR_EQUAL(end, s + 3);
}

void testSTRTOLD(void)
{
    char *end;
    long double ld = strtold("1.25", &end);
    CU_ASSERT_TRUE(ld > 1.24L && ld < 1.26L);
    CU_ASSERT_EQUAL(*end, '\0');

    ld = strtold("1e10", &end);
    CU_ASSERT_TRUE(ld > 9.9e9L && ld < 1.1e10L);
}

/* ---- strtoimax() / strtoumax() (<inttypes.h>) ----------------------- */

void testSTRTOIMAX(void)
{
    char *end;
    CU_ASSERT_EQUAL(strtoimax("12345", &end, 10), (intmax_t)12345);
    CU_ASSERT_EQUAL(strtoimax("-100", &end, 10), (intmax_t)-100);
    CU_ASSERT_EQUAL(strtoimax("ff", &end, 16), (intmax_t)255);

    const char *s = "7q";
    CU_ASSERT_EQUAL(strtoimax(s, &end, 10), (intmax_t)7);
    CU_ASSERT_PTR_EQUAL(end, s + 1);
}

void testSTRTOUMAX(void)
{
    char *end;
    CU_ASSERT_EQUAL(strtoumax("12345", &end, 10), (uintmax_t)12345);
    CU_ASSERT_EQUAL(strtoumax("cafe", &end, 16), (uintmax_t)0xcafe);

    const char *s = "abc";
    CU_ASSERT_EQUAL(strtoumax(s, &end, 10), (uintmax_t)0);
    CU_ASSERT_PTR_EQUAL(end, s);   /* no conversion -> original nptr */
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("strto_C99_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "strtoll() basic", testSTRTOLL_basic)) ||
        (NULL == CU_add_test(pSuite, "strtoll() no conversion", testSTRTOLL_noconv)) ||
        (NULL == CU_add_test(pSuite, "strtoll() overflow", testSTRTOLL_overflow)) ||
        (NULL == CU_add_test(pSuite, "strtoull() basic", testSTRTOULL_basic)) ||
        (NULL == CU_add_test(pSuite, "strtoull() overflow", testSTRTOULL_overflow)) ||
        (NULL == CU_add_test(pSuite, "strtof()", testSTRTOF)) ||
        (NULL == CU_add_test(pSuite, "strtold()", testSTRTOLD)) ||
        (NULL == CU_add_test(pSuite, "strtoimax()", testSTRTOIMAX)) ||
        (NULL == CU_add_test(pSuite, "strtoumax()", testSTRTOUMAX)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-Strto-C99");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

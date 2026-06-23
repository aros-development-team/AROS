/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the C89 (ISO/IEC 9899:1990) string-to-number conversion
    functions strtol(), strtoul() and strtod().

    These exercise the conversion edge cases mandated by the standard:
      - the end pointer equals nptr when no conversion is performed,
      - a "0x" prefix is only consumed when a hex digit follows it,
      - overflow yields LONG_MAX/LONG_MIN/ULONG_MAX with errno == ERANGE.
*/

#include <stdio.h>
#include <stdlib.h>
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

/* ---- strtol() ------------------------------------------------------- */

void testSTRTOL_basic(void)
{
    char *end;

    CU_ASSERT_EQUAL(strtol("123", &end, 10), 123);
    CU_ASSERT_EQUAL(*end, '\0');

    CU_ASSERT_EQUAL(strtol("-42", &end, 10), -42);
    CU_ASSERT_EQUAL(strtol("  +7", &end, 10), 7);

    /* trailing characters: end points just past the digits */
    const char *s = "456abc";
    CU_ASSERT_EQUAL(strtol(s, &end, 10), 456);
    CU_ASSERT_PTR_EQUAL(end, s + 3);
}

void testSTRTOL_bases(void)
{
    char *end;

    CU_ASSERT_EQUAL(strtol("0x1F", &end, 16), 31);
    CU_ASSERT_EQUAL(strtol("0x1F", &end, 0), 31);   /* autodetect hex */
    CU_ASSERT_EQUAL(strtol("010", &end, 0), 8);     /* autodetect octal */
    CU_ASSERT_EQUAL(strtol("10", &end, 0), 10);     /* autodetect decimal */
    CU_ASSERT_EQUAL(strtol("zz", &end, 36), 35 * 36 + 35);
    CU_ASSERT_EQUAL(strtol("100", &end, 2), 4);
}

void testSTRTOL_noconv(void)
{
    char *end;
    const char *s;

    /* No conversion: result is 0 and end == nptr (the original pointer). */
    s = "abc";
    CU_ASSERT_EQUAL(strtol(s, &end, 10), 0);
    CU_ASSERT_PTR_EQUAL(end, s);

    s = "+";
    CU_ASSERT_EQUAL(strtol(s, &end, 10), 0);
    CU_ASSERT_PTR_EQUAL(end, s);

    s = "   ";        /* whitespace only -> original nptr, incl. whitespace */
    CU_ASSERT_EQUAL(strtol(s, &end, 10), 0);
    CU_ASSERT_PTR_EQUAL(end, s);

    s = "99";         /* digits invalid for base 8 */
    CU_ASSERT_EQUAL(strtol(s, &end, 8), 0);
    CU_ASSERT_PTR_EQUAL(end, s);
}

void testSTRTOL_0x_edge(void)
{
    char *end;
    const char *s;

    /* "0x" not followed by a hex digit: the '0' converts, end at 'x'. */
    s = "0x";
    CU_ASSERT_EQUAL(strtol(s, &end, 16), 0);
    CU_ASSERT_PTR_EQUAL(end, s + 1);

    s = "0xZ";
    CU_ASSERT_EQUAL(strtol(s, &end, 16), 0);
    CU_ASSERT_PTR_EQUAL(end, s + 1);
}

void testSTRTOL_overflow(void)
{
    char *end;

    errno = 0;
    CU_ASSERT_EQUAL(strtol("99999999999999999999999", &end, 10), LONG_MAX);
    CU_ASSERT_EQUAL(errno, ERANGE);

    errno = 0;
    CU_ASSERT_EQUAL(strtol("-99999999999999999999999", &end, 10), LONG_MIN);
    CU_ASSERT_EQUAL(errno, ERANGE);
}

/* ---- strtoul() ------------------------------------------------------ */

void testSTRTOUL_basic(void)
{
    char *end;

    CU_ASSERT_EQUAL(strtoul("123", &end, 10), 123UL);
    CU_ASSERT_EQUAL(strtoul("ff", &end, 16), 255UL);
    CU_ASSERT_EQUAL(strtoul("0xcafe", &end, 0), 0xcafeUL);

    /* strtoul() accepts a sign; "-1" wraps to ULONG_MAX. */
    CU_ASSERT_EQUAL(strtoul("-1", &end, 10), ULONG_MAX);
}

void testSTRTOUL_noconv(void)
{
    char *end;
    const char *s = "xyz";
    CU_ASSERT_EQUAL(strtoul(s, &end, 10), 0UL);
    CU_ASSERT_PTR_EQUAL(end, s);
}

void testSTRTOUL_overflow(void)
{
    char *end;

    errno = 0;
    CU_ASSERT_EQUAL(strtoul("99999999999999999999999", &end, 10), ULONG_MAX);
    CU_ASSERT_EQUAL(errno, ERANGE);
}

/* ---- strtod() ------------------------------------------------------- */

static int approx(double a, double b)
{
    double d = a - b;
    if (d < 0) d = -d;
    return d < 1e-9;
}

void testSTRTOD_basic(void)
{
    char *end;

    CU_ASSERT_TRUE(approx(strtod("3.14", &end), 3.14));
    CU_ASSERT_EQUAL(*end, '\0');

    CU_ASSERT_TRUE(approx(strtod("-2.5", &end), -2.5));
    CU_ASSERT_TRUE(approx(strtod("1e3", &end), 1000.0));
    CU_ASSERT_TRUE(approx(strtod("0.5e-1", &end), 0.05));

    const char *s = "1.5xyz";
    CU_ASSERT_TRUE(approx(strtod(s, &end), 1.5));
    CU_ASSERT_PTR_EQUAL(end, s + 3);
}

void testSTRTOD_noconv(void)
{
    char *end;
    const char *s = "abc";
    CU_ASSERT_TRUE(approx(strtod(s, &end), 0.0));
    CU_ASSERT_PTR_EQUAL(end, s);
}

void testSTRTOD_special(void)
{
    char *end;
    const char *s;

    /* "inf" / "infinity" (case-insensitive), C99 7.22.1.3 */
    s = "inf";
    double inf = strtod(s, &end);
    CU_ASSERT_TRUE(isinf(inf));
    CU_ASSERT_TRUE(inf > 0);
    CU_ASSERT_PTR_EQUAL(end, s + 3);

    s = "-INFINITY";
    inf = strtod(s, &end);
    CU_ASSERT_TRUE(isinf(inf));
    CU_ASSERT_TRUE(inf < 0);
    CU_ASSERT_PTR_EQUAL(end, s + 9);

    /* "nan" */
    s = "nan";
    double nan = strtod(s, &end);
    CU_ASSERT_TRUE(isnan(nan));
    CU_ASSERT_PTR_EQUAL(end, s + 3);

    /* nan with an (n-char-sequence) */
    s = "NAN(123)x";
    nan = strtod(s, &end);
    CU_ASSERT_TRUE(isnan(nan));
    CU_ASSERT_PTR_EQUAL(end, s + 8);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("strto_C89_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "strtol() basic", testSTRTOL_basic)) ||
        (NULL == CU_add_test(pSuite, "strtol() bases", testSTRTOL_bases)) ||
        (NULL == CU_add_test(pSuite, "strtol() no conversion", testSTRTOL_noconv)) ||
        (NULL == CU_add_test(pSuite, "strtol() 0x edge", testSTRTOL_0x_edge)) ||
        (NULL == CU_add_test(pSuite, "strtol() overflow", testSTRTOL_overflow)) ||
        (NULL == CU_add_test(pSuite, "strtoul() basic", testSTRTOUL_basic)) ||
        (NULL == CU_add_test(pSuite, "strtoul() no conversion", testSTRTOUL_noconv)) ||
        (NULL == CU_add_test(pSuite, "strtoul() overflow", testSTRTOUL_overflow)) ||
        (NULL == CU_add_test(pSuite, "strtod() basic", testSTRTOD_basic)) ||
        (NULL == CU_add_test(pSuite, "strtod() no conversion", testSTRTOD_noconv)) ||
        (NULL == CU_add_test(pSuite, "strtod() inf/nan", testSTRTOD_special)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-Strto-C89");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

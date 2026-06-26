/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the C89 (ISO/IEC 9899:1990) formatted input functions,
    exercised through sscanf().  Coverage includes the integer/float/string
    conversions, the %i base auto-detection, field-width limits (which must
    bound both the number of characters consumed and stored), scan-sets,
    assignment suppression, the %n directive and the matched-item return
    count (C89 7.9.6.2).
*/

#include <stdio.h>
#include <string.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

int init_suite(void) { return 0; }
int clean_suite(void) { return 0; }

void test_scanf_decimal(void)
{
    int a = 0, b = 0, n;

    n = sscanf("123 -456", "%d %d", &a, &b);
    CU_ASSERT_EQUAL(n, 2);
    CU_ASSERT_EQUAL(a, 123);
    CU_ASSERT_EQUAL(b, -456);
}

/* %i auto-detects base: 0x.. hex, 0.. octal, otherwise decimal. */
void test_scanf_i_bases(void)
{
    int v;

    v = 0; CU_ASSERT_EQUAL(sscanf("0xAF", "%i", &v), 1);
    CU_ASSERT_EQUAL(v, 0xAF);

    v = 0; CU_ASSERT_EQUAL(sscanf("0x10", "%i", &v), 1);
    CU_ASSERT_EQUAL(v, 16);

    v = 0; CU_ASSERT_EQUAL(sscanf("017", "%i", &v), 1);
    CU_ASSERT_EQUAL(v, 15);     /* octal */

    v = 0; CU_ASSERT_EQUAL(sscanf("42", "%i", &v), 1);
    CU_ASSERT_EQUAL(v, 42);     /* decimal */

    /* "AF" without a 0x prefix is not a number for %i. */
    v = 999; CU_ASSERT_EQUAL(sscanf("AF", "%i", &v), 0);
}

void test_scanf_hex_octal(void)
{
    unsigned int x = 0, o = 0;

    CU_ASSERT_EQUAL(sscanf("ff", "%x", &x), 1);
    CU_ASSERT_EQUAL(x, 0xff);

    CU_ASSERT_EQUAL(sscanf("17", "%o", &o), 1);
    CU_ASSERT_EQUAL(o, 017);
}

/* A field width bounds the number of characters consumed for numeric
   conversions: "%2d" on "12345" reads only "12". */
void test_scanf_width_int(void)
{
    int a = 0, b = 0, n;

    n = sscanf("12345", "%2d%3d", &a, &b);
    CU_ASSERT_EQUAL(n, 2);
    CU_ASSERT_EQUAL(a, 12);
    CU_ASSERT_EQUAL(b, 345);
}

/* "%3s" stores at most 3 characters plus the terminating NUL, and stops
   consuming after the third. */
void test_scanf_width_str(void)
{
    char buf[8];
    char rest[8];
    int n;

    memset(buf, 'Z', sizeof(buf));
    n = sscanf("hello", "%3s", buf);
    CU_ASSERT_EQUAL(n, 1);
    CU_ASSERT_STRING_EQUAL(buf, "hel");

    /* The unconsumed remainder must still be available to a second field. */
    n = sscanf("hello", "%3s%3s", buf, rest);
    CU_ASSERT_EQUAL(n, 2);
    CU_ASSERT_STRING_EQUAL(buf, "hel");
    CU_ASSERT_STRING_EQUAL(rest, "lo");
}

/* "%1c" reads exactly one character and leaves the next one for the
   following directive (field width bounds consumption). */
void test_scanf_char_width(void)
{
    char c1 = 0, c2 = 0;
    int n;

    n = sscanf("ab", "%1c%1c", &c1, &c2);
    CU_ASSERT_EQUAL(n, 2);
    CU_ASSERT_EQUAL(c1, 'a');
    CU_ASSERT_EQUAL(c2, 'b');
}

void test_scanf_scanset(void)
{
    char buf[16];
    int n;

    memset(buf, 0, sizeof(buf));
    n = sscanf("12345abc", "%[0-9]", buf);
    CU_ASSERT_EQUAL(n, 1);
    CU_ASSERT_STRING_EQUAL(buf, "12345");

    /* Negated scan-set stops at the first listed character. */
    memset(buf, 0, sizeof(buf));
    n = sscanf("hello,world", "%[^,]", buf);
    CU_ASSERT_EQUAL(n, 1);
    CU_ASSERT_STRING_EQUAL(buf, "hello");
}

/* Assignment suppression: %*d consumes but does not store. */
void test_scanf_suppress(void)
{
    int a = 0, n;

    n = sscanf("11 22 33", "%*d %d", &a);
    CU_ASSERT_EQUAL(n, 1);
    CU_ASSERT_EQUAL(a, 22);
}

/* %n stores the number of characters consumed so far and is not counted in
   the return value. */
void test_scanf_n(void)
{
    int a = 0, consumed = -1, n;

    n = sscanf("123abc", "%d%n", &a, &consumed);
    CU_ASSERT_EQUAL(n, 1);
    CU_ASSERT_EQUAL(a, 123);
    CU_ASSERT_EQUAL(consumed, 3);
}

/* A failed match returns the count of items assigned before the failure;
   EOF before any conversion returns EOF. */
void test_scanf_returns(void)
{
    int a = 0, b = 0;

    CU_ASSERT_EQUAL(sscanf("123 abc", "%d %d", &a, &b), 1);
    CU_ASSERT_EQUAL(a, 123);

    CU_ASSERT_EQUAL(sscanf("", "%d", &a), EOF);
}

void test_scanf_float(void)
{
    float f = 0.0f;

    CU_ASSERT_EQUAL(sscanf("3.14", "%f", &f), 1);
    CU_ASSERT_DOUBLE_EQUAL(f, 3.14f, 0.0001);

    CU_ASSERT_EQUAL(sscanf("-.5", "%f", &f), 1);
    CU_ASSERT_DOUBLE_EQUAL(f, -0.5f, 0.0001);
}

/* The ll length modifier must read full 64-bit values, not truncate to the
   32-bit range (C99 7.19.6.2).  Regression guard for %llu/%lld/%llx. */
void test_scanf_longlong(void)
{
    unsigned long long u = 0;
    long long s = 0;

    /* ULLONG_MAX must round-trip without being capped at 0xFFFFFFFF. */
    CU_ASSERT_EQUAL(sscanf("18446744073709551615", "%llu", &u), 1);
    CU_ASSERT_TRUE(u == 18446744073709551615ULL);

    /* A value above the 32-bit range. */
    u = 0;
    CU_ASSERT_EQUAL(sscanf("4294967296", "%llu", &u), 1);
    CU_ASSERT_TRUE(u == 4294967296ULL);

    /* Signed long long, including a large negative value. */
    CU_ASSERT_EQUAL(sscanf("-9223372036854775807", "%lld", &s), 1);
    CU_ASSERT_TRUE(s == -9223372036854775807LL);

    /* Hexadecimal long long. */
    u = 0;
    CU_ASSERT_EQUAL(sscanf("ffffffffffffffff", "%llx", &u), 1);
    CU_ASSERT_TRUE(u == 0xFFFFFFFFFFFFFFFFULL);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("SCANF_C89_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "decimal", test_scanf_decimal)) ||
        (NULL == CU_add_test(pSuite, "i_bases", test_scanf_i_bases)) ||
        (NULL == CU_add_test(pSuite, "hex_octal", test_scanf_hex_octal)) ||
        (NULL == CU_add_test(pSuite, "width_int", test_scanf_width_int)) ||
        (NULL == CU_add_test(pSuite, "width_str", test_scanf_width_str)) ||
        (NULL == CU_add_test(pSuite, "char_width", test_scanf_char_width)) ||
        (NULL == CU_add_test(pSuite, "scanset", test_scanf_scanset)) ||
        (NULL == CU_add_test(pSuite, "suppress", test_scanf_suppress)) ||
        (NULL == CU_add_test(pSuite, "n_directive", test_scanf_n)) ||
        (NULL == CU_add_test(pSuite, "returns", test_scanf_returns)) ||
        (NULL == CU_add_test(pSuite, "float", test_scanf_float)) ||
        (NULL == CU_add_test(pSuite, "longlong", test_scanf_longlong)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-SCANF-C89");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

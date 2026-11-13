/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the formatted output functions snprintf() (narrow) and
    swprintf() (wide).  Coverage focuses on the cross-width string/character
    conversions (%ls/%S/%lc/%C in a narrow stream and %s/%c in a wide stream),
    the field-width and precision handling of those conversions, the ll length
    modifier for 64-bit integers, and the wide formatter in general (which must
    not read its flag/number scratch arrays out of bounds).

    ASCII-only data is used so the multibyte<->wide conversions are well
    defined in the default "C" locale.
*/

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

int init_suite(void) { return 0; }
int clean_suite(void) { return 0; }

/* ---- narrow snprintf -------------------------------------------------- */

/* snprintf() returns the length that would have been written and always NUL
   terminates within the buffer (C99 7.19.6.5). */
void test_snprintf_return_truncate(void)
{
    char buf[8];
    int n;

    n = snprintf(buf, sizeof(buf), "%s", "abc");
    CU_ASSERT_EQUAL(n, 3);
    CU_ASSERT_STRING_EQUAL(buf, "abc");

    /* Truncation: return value is the untruncated length, buffer is capped. */
    n = snprintf(buf, sizeof(buf), "%s", "0123456789");
    CU_ASSERT_EQUAL(n, 10);
    CU_ASSERT_EQUAL(buf[7], '\0');
    CU_ASSERT_EQUAL(strlen(buf), 7);
}

/* %ls and %S print a whole wchar_t string (not just the first character). */
void test_snprintf_wide_string(void)
{
    static const wchar_t w[] = { L'h', L'e', L'l', L'l', L'o', 0 };
    char buf[32];

    CU_ASSERT_EQUAL(snprintf(buf, sizeof(buf), "%ls", w), 5);
    CU_ASSERT_STRING_EQUAL(buf, "hello");

    CU_ASSERT_EQUAL(snprintf(buf, sizeof(buf), "%S", w), 5);
    CU_ASSERT_STRING_EQUAL(buf, "hello");
}

/* Field width and precision apply to the converted %ls output. */
void test_snprintf_wide_string_width(void)
{
    static const wchar_t w[] = { L'h', L'i', 0 };
    static const wchar_t wlong[] = { L't', L'r', L'u', L'n', L'c', 0 };
    char buf[32];

    snprintf(buf, sizeof(buf), "%5ls", w);
    CU_ASSERT_STRING_EQUAL(buf, "   hi");

    snprintf(buf, sizeof(buf), "%-5ls|", w);
    CU_ASSERT_STRING_EQUAL(buf, "hi   |");

    snprintf(buf, sizeof(buf), "%.3ls", wlong);
    CU_ASSERT_STRING_EQUAL(buf, "tru");
}

/* %lc and %C print a wide character. */
void test_snprintf_wide_char(void)
{
    char buf[8];

    CU_ASSERT_EQUAL(snprintf(buf, sizeof(buf), "%lc", (wint_t)L'A'), 1);
    CU_ASSERT_STRING_EQUAL(buf, "A");

    CU_ASSERT_EQUAL(snprintf(buf, sizeof(buf), "[%C]", (wint_t)L'Z'), 3);
    CU_ASSERT_STRING_EQUAL(buf, "[Z]");
}

/* The ll length modifier must format full 64-bit integers. */
void test_snprintf_longlong(void)
{
    char buf[32];

    snprintf(buf, sizeof(buf), "%llu", 18446744073709551615ULL);
    CU_ASSERT_STRING_EQUAL(buf, "18446744073709551615");

    snprintf(buf, sizeof(buf), "%lld", -9223372036854775807LL);
    CU_ASSERT_STRING_EQUAL(buf, "-9223372036854775807");

    snprintf(buf, sizeof(buf), "%llx", 0xFFFFFFFFFFFFFFFFULL);
    CU_ASSERT_STRING_EQUAL(buf, "ffffffffffffffff");
}

/* Basic floating point formatting (values chosen to be free of half-way
   rounding ambiguity). */
void test_snprintf_float(void)
{
    char buf[64];

    snprintf(buf, sizeof(buf), "%6.3f %6.3f", 3.14, 6.28);
    CU_ASSERT_STRING_EQUAL(buf, " 3.140  6.280");

    snprintf(buf, sizeof(buf), "%.2f", 3.14159);
    CU_ASSERT_STRING_EQUAL(buf, "3.14");

    snprintf(buf, sizeof(buf), "%.4f", 3.14159);
    CU_ASSERT_STRING_EQUAL(buf, "3.1416");

    /* Exactly representable values are printed exactly at high precision. */
    snprintf(buf, sizeof(buf), "%.10f", 0.25);
    CU_ASSERT_STRING_EQUAL(buf, "0.2500000000");

    snprintf(buf, sizeof(buf), "%f", 1.5);
    CU_ASSERT_STRING_EQUAL(buf, "1.500000");
}

/* ---- wide swprintf ---------------------------------------------------- */

/* General wide formatting must work - a regression guard against the wide
   formatter reading its flag/number scratch arrays out of bounds (which
   previously corrupted or hung every swprintf() call). */
void test_swprintf_basic(void)
{
    wchar_t buf[64];

    CU_ASSERT_EQUAL(swprintf(buf, 64, L"%d-%x", 42, 255), 5);
    CU_ASSERT_TRUE(wcscmp(buf, L"42-ff") == 0);

    swprintf(buf, 64, L"%05d", 7);
    CU_ASSERT_TRUE(wcscmp(buf, L"00007") == 0);

    swprintf(buf, 64, L"%+d|% d", 5, 5);
    CU_ASSERT_TRUE(wcscmp(buf, L"+5| 5") == 0);

    swprintf(buf, 64, L"%-6d|", 3);
    CU_ASSERT_TRUE(wcscmp(buf, L"3     |") == 0);

    swprintf(buf, 64, L"100%%");
    CU_ASSERT_TRUE(wcscmp(buf, L"100%") == 0);
}

/* Wide %ls prints a wchar_t string; wide %s converts a narrow (multibyte)
   string to wide characters. */
void test_swprintf_strings(void)
{
    wchar_t buf[64];

    swprintf(buf, 64, L"%ls", L"wide");
    CU_ASSERT_TRUE(wcscmp(buf, L"wide") == 0);

    swprintf(buf, 64, L"%s", "narrow");
    CU_ASSERT_TRUE(wcscmp(buf, L"narrow") == 0);

    swprintf(buf, 64, L"%8s", "hi");
    CU_ASSERT_TRUE(wcscmp(buf, L"      hi") == 0);

    swprintf(buf, 64, L"%.3s", "truncate");
    CU_ASSERT_TRUE(wcscmp(buf, L"tru") == 0);
}

/* Wide %c emits a (narrow) character widened; %lc emits a wide character. */
void test_swprintf_chars(void)
{
    wchar_t buf[16];

    swprintf(buf, 16, L"%c", 'Q');
    CU_ASSERT_TRUE(wcscmp(buf, L"Q") == 0);

    swprintf(buf, 16, L"[%lc]", (wint_t)L'W');
    CU_ASSERT_TRUE(wcscmp(buf, L"[W]") == 0);
}

/* Wide floating point formatting. */
void test_swprintf_float(void)
{
    wchar_t buf[64];

    swprintf(buf, 64, L"%.3f", 3.14159);
    CU_ASSERT_TRUE(wcscmp(buf, L"3.142") == 0);

    swprintf(buf, 64, L"%.2f", 1.5);
    CU_ASSERT_TRUE(wcscmp(buf, L"1.50") == 0);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("PRINTF_C99_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "snprintf_return_truncate", test_snprintf_return_truncate)) ||
        (NULL == CU_add_test(pSuite, "snprintf_wide_string", test_snprintf_wide_string)) ||
        (NULL == CU_add_test(pSuite, "snprintf_wide_string_width", test_snprintf_wide_string_width)) ||
        (NULL == CU_add_test(pSuite, "snprintf_wide_char", test_snprintf_wide_char)) ||
        (NULL == CU_add_test(pSuite, "snprintf_longlong", test_snprintf_longlong)) ||
        (NULL == CU_add_test(pSuite, "snprintf_float", test_snprintf_float)) ||
        (NULL == CU_add_test(pSuite, "swprintf_basic", test_swprintf_basic)) ||
        (NULL == CU_add_test(pSuite, "swprintf_strings", test_swprintf_strings)) ||
        (NULL == CU_add_test(pSuite, "swprintf_chars", test_swprintf_chars)) ||
        (NULL == CU_add_test(pSuite, "swprintf_float", test_swprintf_float)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-PRINTF-C99");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the POSIX/XSI character-cell width functions
    wcwidth()/wcswidth(), provided by posixc.library.
*/

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

int init_suite(void) { return 0; }
int clean_suite(void) { return 0; }

void test_wcwidth(void)
{
    /* NUL is zero-width */
    CU_ASSERT_EQUAL(wcwidth(L'\0'), 0);

    /* Ordinary printable ASCII is single-width */
    CU_ASSERT_EQUAL(wcwidth(L'A'), 1);
    CU_ASSERT_EQUAL(wcwidth(L' '), 1);
    CU_ASSERT_EQUAL(wcwidth(L'~'), 1);

    /* C0 / C1 control characters are non-printable (-1) */
    CU_ASSERT_EQUAL(wcwidth((wchar_t)0x1B), -1);  /* ESC  */
    CU_ASSERT_EQUAL(wcwidth((wchar_t)0x7F), -1);  /* DEL  */
    CU_ASSERT_EQUAL(wcwidth((wchar_t)0x80), -1);  /* C1   */

    /* Combining marks are zero-width */
    CU_ASSERT_EQUAL(wcwidth((wchar_t)0x0301), 0); /* combining acute accent */
    CU_ASSERT_EQUAL(wcwidth((wchar_t)0x0300), 0); /* combining grave accent */
    CU_ASSERT_EQUAL(wcwidth((wchar_t)0x200B), 0); /* zero width space        */

    /* East Asian wide / fullwidth characters are double-width */
    CU_ASSERT_EQUAL(wcwidth((wchar_t)0x1100), 2); /* Hangul Jamo            */
    CU_ASSERT_EQUAL(wcwidth((wchar_t)0x4E00), 2); /* CJK ideograph          */
    CU_ASSERT_EQUAL(wcwidth((wchar_t)0xAC00), 2); /* Hangul syllable        */
    CU_ASSERT_EQUAL(wcwidth((wchar_t)0xFF01), 2); /* fullwidth exclamation  */
}

void test_wcswidth(void)
{
    wchar_t ascii[]    = { L'A', L'B', L'C', 0 };
    wchar_t cjk[]      = { 0x4E00, 0x4E01, 0 };
    wchar_t combining[]= { L'A', 0x0301, L'B', 0 };
    wchar_t withctrl[] = { L'A', 0x1B, L'B', 0 };

    /* Three single-width cells */
    CU_ASSERT_EQUAL(wcswidth(ascii, 3), 3);

    /* 'n' caps the number of characters examined */
    CU_ASSERT_EQUAL(wcswidth(ascii, 2), 2);
    CU_ASSERT_EQUAL(wcswidth(ascii, 100), 3); /* stops at the NUL */

    /* Two double-width cells */
    CU_ASSERT_EQUAL(wcswidth(cjk, 2), 4);

    /* Base + combining mark collapses to a single cell */
    CU_ASSERT_EQUAL(wcswidth(combining, 3), 2);

    /* A non-printable character makes the whole string -1 */
    CU_ASSERT_EQUAL(wcswidth(withctrl, 3), -1);

    /* Empty span is zero */
    CU_ASSERT_EQUAL(wcswidth(ascii, 0), 0);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("WCWIDTH_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "wcwidth", test_wcwidth)) ||
        (NULL == CU_add_test(pSuite, "wcswidth", test_wcswidth)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-PosixC-WCWIDTH");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

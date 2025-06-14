/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <wchar.h>

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

void test_wcrtomb_and_wctomb(void)
{
    char buf[5];
    memset(buf, 0, sizeof(buf));

    wchar_t wc = 0x00E4; // 'ä'
    size_t len = wcrtomb(buf, wc, NULL);

    CU_ASSERT_EQUAL(len, 2);
    CU_ASSERT_EQUAL((unsigned char)buf[0], 0xC3);
    CU_ASSERT_EQUAL((unsigned char)buf[1], 0xA4);

    // legacy wrapper
    memset(buf, 0, sizeof(buf));
    int len2 = wctomb(buf, wc);
    CU_ASSERT_EQUAL(len2, 2);
    CU_ASSERT_EQUAL((unsigned char)buf[0], 0xC3);
    CU_ASSERT_EQUAL((unsigned char)buf[1], 0xA4);
}

void test_mbtowc_and_mblen(void)
{
    const char *s = "ä"; // UTF-8: C3 A4
    wchar_t wc;
    int len = mbtowc(&wc, s, 2);

    CU_ASSERT_EQUAL(len, 2);
    CU_ASSERT_EQUAL(wc, 0x00E4);

    // mblen variant
    int mlen = mblen(s, 2);
    CU_ASSERT_EQUAL(mlen, 2);
}

void test_wcstombs_and_mbstowcs(void)
{
    const wchar_t wstr[] = { 0x0041, 0x00E4, 0x20AC, 0 }; // "Aä€"
    char mbbuf[16];
    memset(mbbuf, 0, sizeof(mbbuf));

    size_t mblen = wcstombs(mbbuf, wstr, sizeof(mbbuf));
    CU_ASSERT(mblen > 0);
    CU_ASSERT_STRING_EQUAL(mbbuf, "Aä€");

    wchar_t wbuf[8];
    memset(wbuf, 0, sizeof(wbuf));
    size_t wlen = mbstowcs(wbuf, mbbuf, 8);
    CU_ASSERT(wlen == 3);
    CU_ASSERT(wbuf[0] == 0x0041);
    CU_ASSERT(wbuf[1] == 0x00E4);
    CU_ASSERT(wbuf[2] == 0x20AC);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

   /* add a suite to the registry */
    pSuite = CU_add_suite("StandardTypes_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

   /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "wcrtomb & wctomb", test_wcrtomb_and_wctomb)) ||
        (NULL == CU_add_test(pSuite, "mbtowc & mblen", test_mbtowc_and_mblen)) ||
        (NULL == CU_add_test(pSuite, "wcstombs & mbstowcs", test_wcstombs_and_mbstowcs)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-wchar");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

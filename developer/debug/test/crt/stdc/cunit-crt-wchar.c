/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

/* ASCII-only test string safe in "C" locale */
static const char *ascii_str = "AROS Rocks";
static wchar_t ascii_wcs[] = {
    L'A', L'R', L'O', L'S', L' ',
    L'R', L'o', L'c', L'k', L's',
    0
};

/* UTF-8 test string with non-ASCII chars (ö and check mark) */
static const char *utf8_str = "AROS R"
    "\xC3\xB6"
    "cks "
    "\xE2\x9C\x93";
static wchar_t utf8_wcs[] = {
    L'A', L'R', L'O', L'S', L' ',
    L'R', 0x00F6, L'c', L'k', L's', L' ',
    0x2713, /* check mark */
    0
};

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
/* Helper: Return pointer to appropriate test string based on locale */
static const char *get_test_str(void) {
    const char *locale = setlocale(LC_CTYPE, NULL);

    if (locale == NULL || strcmp(locale, "C") == 0) {
        return ascii_str;
    }
    return utf8_str;
}

/* Helper: Return pointer to appropriate wide string based on locale */
static const wchar_t *get_test_wcs(void) {
    const char *locale = setlocale(LC_CTYPE, NULL);

    if (locale == NULL || strcmp(locale, "C") == 0) {
        return ascii_wcs;
    }
    return utf8_wcs;
}

void test_mblen(void)
{
    const char *s = "\xC3\xA4"; // "ä" UTF-8

    int mlen = mblen(s, 2);

    CU_ASSERT_EQUAL(mlen, 2);
}

/* Test mbrlen with single multibyte characters */
void test_mbrlen(void) {
    mbstate_t state = {0};
    const char *s = get_test_str();
    size_t len;

    /* Test 1: first character length */
    len = mbrlen(s, MB_CUR_MAX, &state);
    CU_ASSERT(len > 0);

    /* Test 2: NULL input resets state, returns 0 or positive */
    len = mbrlen(NULL, MB_CUR_MAX, &state);
    CU_ASSERT(len == 0 || len == (size_t)-2); /* Some implementations */
}

void test_mbtowc(void)
{
    const char *s = "\xC3\xA4"; // "ä" UTF-8
    wchar_t wc;
    int len = mbtowc(&wc, s, 2);

    CU_ASSERT_EQUAL(len, 2);
    CU_ASSERT_EQUAL(wc, 0x00E4);
}

/* Test mbstowcs conversion */
void test_mbstowcs(void) {
    const char *src = get_test_str();
    wchar_t dest[64];
    size_t n = sizeof(dest)/sizeof(dest[0]);

    size_t ret = mbstowcs(dest, src, n);
    CU_ASSERT(ret != (size_t)-1);

    const wchar_t *expected = get_test_wcs();
    CU_ASSERT(wcscmp(dest, expected) == 0);
}

/* Test mbsrtowcs conversion */
void test_mbsrtowcs(void) {
    const char *src_str = get_test_str();
    const char *src = src_str;
    wchar_t dest[64];
    size_t n = sizeof(dest)/sizeof(dest[0]);

    size_t ret = mbsrtowcs(dest, &src, n, NULL);
    CU_ASSERT(ret != (size_t)-1);

    const wchar_t *expected = get_test_wcs();
    CU_ASSERT(wcscmp(dest, expected) == 0);
    CU_ASSERT(src == NULL || *src == '\0'); /* Should consume whole string */
}

void test_wctomb(void)
{
    char buf[5];
    memset(buf, 0, sizeof(buf));

    wchar_t wc = 0x00E4; // 'ä'
    int len2 = wctomb(buf, wc);

    CU_ASSERT_EQUAL(len2, 2);
    CU_ASSERT_EQUAL((unsigned char)buf[0], 0xC3);
    CU_ASSERT_EQUAL((unsigned char)buf[1], 0xA4);
}

void test_wcrtomb(void)
{
    char buf[5];
    memset(buf, 0, sizeof(buf));

    wchar_t wc = 0x00E4; // 'ä'
    size_t len = wcrtomb(buf, wc, NULL);

    CU_ASSERT_EQUAL(len, 2);
    CU_ASSERT_EQUAL((unsigned char)buf[0], 0xC3);
    CU_ASSERT_EQUAL((unsigned char)buf[1], 0xA4);
}

void test_wcstombs(void)
{
    const wchar_t *src = get_test_wcs();
    const wchar_t *src_ptr = src;
    char dest[128];
    memset(dest, 0, sizeof(dest));
    size_t n = sizeof(dest);

    size_t ret = wcstombs(dest, src_ptr, n);
    CU_ASSERT(ret > 0);

    const char *expected = get_test_str();
    CU_ASSERT(strcmp(dest, expected) == 0);
    CU_ASSERT(src_ptr == NULL || *src_ptr == L'\0'); /* Should consume whole string */
}

/* Test wcsrtombs conversion */
void test_wcsrtombs(void) {
    const wchar_t *src = get_test_wcs();
    const wchar_t *src_ptr = src;
    char dest[128];
    size_t n = sizeof(dest);

    size_t ret = wcsrtombs(dest, &src_ptr, n, NULL);
    CU_ASSERT(ret != (size_t)-1);

    const char *expected = get_test_str();
    CU_ASSERT(strcmp(dest, expected) == 0);
    CU_ASSERT(src_ptr == NULL || *src_ptr == L'\0'); /* Should consume whole string */
}

/* Test towlower and towupper */
void test_towcase(void) {
    const wchar_t *wstr = get_test_wcs();
    wchar_t lower[64], upper[64];
    size_t i;
#if (0)
    for (i = 0; wstr[i] != 0 && i < 63; i++) {
        lower[i] = towlower(wstr[i]);
        upper[i] = towupper(wstr[i]);
    }
    lower[i] = 0;
    upper[i] = 0;

    /* For ASCII, check case conversion correctness */
    if (strcmp(setlocale(LC_CTYPE, NULL), "C") == 0) {
        /* "AROS Rocks" tolower should be "aros rocks" */
        CU_ASSERT_STRING_EQUAL(lower, L"aros rocks");
        /* toupper should be "AROS ROCKS" */
        CU_ASSERT_STRING_EQUAL(upper, L"AROS ROCKS");
    }
#endif
}

/* Test isw* functions */
void test_iswctype(void) {
    const wchar_t *wstr = get_test_wcs();
    size_t i;

#if (0)
    for (i = 0; wstr[i] != 0; i++) {
        CU_ASSERT(iswalnum(wstr[i]) || iswspace(wstr[i]) || iswpunct(wstr[i]) || iswcntrl(wstr[i]));
    }
#endif
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

   /* add a suite to the registry */
    pSuite = CU_add_suite("WCHAR_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

   /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "mblen", test_mblen)) ||
    (NULL == CU_add_test(pSuite, "mbrlen", test_mbrlen)) ||
    (NULL == CU_add_test(pSuite, "mbtowc", test_mbtowc)) ||
    (NULL == CU_add_test(pSuite, "mbstowcs", test_mbstowcs)) ||
    (NULL == CU_add_test(pSuite, "mbsrtowcs", test_mbsrtowcs)) ||
    (NULL == CU_add_test(pSuite, "wctomb", test_wctomb)) ||
    (NULL == CU_add_test(pSuite, "wcrtomb", test_wcrtomb)) ||
    (NULL == CU_add_test(pSuite, "wcstombs", test_wcstombs)) ||
    (NULL == CU_add_test(pSuite, "wcsrtombs", test_wcsrtombs)) ||
    (NULL == CU_add_test(pSuite, "towcase", test_towcase)) ||
    (NULL == CU_add_test(pSuite, "iswctype", test_iswctype)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-WCHAR");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

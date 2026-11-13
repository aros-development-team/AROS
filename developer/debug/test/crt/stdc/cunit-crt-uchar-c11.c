/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the ISO C11 <uchar.h> Unicode conversion functions
    (mbrtoc16/c16rtomb/mbrtoc32/c32rtomb), provided by stdc.library.

    The AROS multibyte conversion is locale-independent UTF-8, so the byte
    sequences below decode the same regardless of the active locale.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <uchar.h>
#include <wchar.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

int init_suite(void) { return 0; }
int clean_suite(void) { return 0; }

/* mbrtoc32(): a char32_t holds a full code point. */
void test_mbrtoc32(void)
{
    char32_t c32 = 0;
    mbstate_t ps;
    size_t r;

    /* ASCII */
    memset(&ps, 0, sizeof(ps));
    r = mbrtoc32(&c32, "A", 1, &ps);
    CU_ASSERT_EQUAL(r, 1);
    CU_ASSERT_EQUAL(c32, (char32_t)'A');

    /* NUL character returns 0 */
    memset(&ps, 0, sizeof(ps));
    c32 = 0xFFFF;
    r = mbrtoc32(&c32, "\0", 1, &ps);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_EQUAL(c32, 0);

    /* 2-byte UTF-8: U+00F6 (o-umlaut) = C3 B6 */
    memset(&ps, 0, sizeof(ps));
    r = mbrtoc32(&c32, "\xC3\xB6", 2, &ps);
    CU_ASSERT_EQUAL(r, 2);
    CU_ASSERT_EQUAL(c32, 0x00F6);

    /* 4-byte UTF-8: U+1F600 = F0 9F 98 80 */
    memset(&ps, 0, sizeof(ps));
    r = mbrtoc32(&c32, "\xF0\x9F\x98\x80", 4, &ps);
    CU_ASSERT_EQUAL(r, 4);
    CU_ASSERT_EQUAL(c32, 0x1F600);

    /* Incomplete sequence */
    memset(&ps, 0, sizeof(ps));
    r = mbrtoc32(&c32, "\xC3", 1, &ps);
    CU_ASSERT_EQUAL(r, (size_t)-2);
}

/* c32rtomb(): inverse of mbrtoc32(). */
void test_c32rtomb(void)
{
    char buf[8];
    mbstate_t ps;
    size_t r;

    memset(&ps, 0, sizeof(ps));
    r = c32rtomb(buf, (char32_t)'A', &ps);
    CU_ASSERT_EQUAL(r, 1);
    CU_ASSERT_EQUAL(buf[0], 'A');

    /* U+00F6 -> 2 bytes */
    memset(&ps, 0, sizeof(ps));
    r = c32rtomb(buf, 0x00F6, &ps);
    CU_ASSERT_EQUAL(r, 2);
    CU_ASSERT_EQUAL((unsigned char)buf[0], 0xC3);
    CU_ASSERT_EQUAL((unsigned char)buf[1], 0xB6);

    /* U+1F600 -> 4 bytes */
    memset(&ps, 0, sizeof(ps));
    r = c32rtomb(buf, 0x1F600, &ps);
    CU_ASSERT_EQUAL(r, 4);
    CU_ASSERT_EQUAL((unsigned char)buf[0], 0xF0);
    CU_ASSERT_EQUAL((unsigned char)buf[1], 0x9F);
    CU_ASSERT_EQUAL((unsigned char)buf[2], 0x98);
    CU_ASSERT_EQUAL((unsigned char)buf[3], 0x80);
}

/* mbrtoc16(): code points beyond the BMP yield a UTF-16 surrogate pair. */
void test_mbrtoc16(void)
{
    char16_t c16 = 0;
    mbstate_t ps;
    size_t r;

    /* BMP character: single 16-bit unit */
    memset(&ps, 0, sizeof(ps));
    r = mbrtoc16(&c16, "\xC3\xB6", 2, &ps);
    CU_ASSERT_EQUAL(r, 2);
    CU_ASSERT_EQUAL(c16, 0x00F6);

    /* Astral character U+1F600: high surrogate now (bytes consumed)... */
    memset(&ps, 0, sizeof(ps));
    r = mbrtoc16(&c16, "\xF0\x9F\x98\x80", 4, &ps);
    CU_ASSERT_EQUAL(r, 4);
    CU_ASSERT_EQUAL(c16, 0xD83D);

    /* ...then the low surrogate, consuming no further input (-3). */
    r = mbrtoc16(&c16, "", 0, &ps);
    CU_ASSERT_EQUAL(r, (size_t)-3);
    CU_ASSERT_EQUAL(c16, 0xDE00);
}

/* c16rtomb(): a buffered high surrogate is combined with the low surrogate. */
void test_c16rtomb(void)
{
    char buf[8];
    mbstate_t ps;
    size_t r;

    /* BMP character */
    memset(&ps, 0, sizeof(ps));
    r = c16rtomb(buf, 0x00F6, &ps);
    CU_ASSERT_EQUAL(r, 2);
    CU_ASSERT_EQUAL((unsigned char)buf[0], 0xC3);
    CU_ASSERT_EQUAL((unsigned char)buf[1], 0xB6);

    /* High surrogate: buffered, no output yet */
    memset(&ps, 0, sizeof(ps));
    r = c16rtomb(buf, 0xD83D, &ps);
    CU_ASSERT_EQUAL(r, 0);

    /* Low surrogate: completes the pair into U+1F600 (4 bytes) */
    r = c16rtomb(buf, 0xDE00, &ps);
    CU_ASSERT_EQUAL(r, 4);
    CU_ASSERT_EQUAL((unsigned char)buf[0], 0xF0);
    CU_ASSERT_EQUAL((unsigned char)buf[1], 0x9F);
    CU_ASSERT_EQUAL((unsigned char)buf[2], 0x98);
    CU_ASSERT_EQUAL((unsigned char)buf[3], 0x80);

    /* Unpaired low surrogate is an encoding error */
    memset(&ps, 0, sizeof(ps));
    errno = 0;
    r = c16rtomb(buf, 0xDE00, &ps);
    CU_ASSERT_EQUAL(r, (size_t)-1);
    CU_ASSERT_EQUAL(errno, EILSEQ);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("UCHAR_C11_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "mbrtoc32", test_mbrtoc32)) ||
        (NULL == CU_add_test(pSuite, "c32rtomb", test_c32rtomb)) ||
        (NULL == CU_add_test(pSuite, "mbrtoc16", test_mbrtoc16)) ||
        (NULL == CU_add_test(pSuite, "c16rtomb", test_c16rtomb)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-UCHAR-C11");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

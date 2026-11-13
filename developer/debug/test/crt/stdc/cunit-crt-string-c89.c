/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the C89 (ISO/IEC 9899:1990) <string.h> functions:
    copying, concatenation, comparison, search and the mem* block helpers.
*/

#include <stdio.h>
#include <string.h>

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

/* ---- copying -------------------------------------------------------- */

void testSTRCPY(void)
{
    char buf[16];
    char *r = strcpy(buf, "hello");
    CU_ASSERT_PTR_EQUAL(r, buf);
    CU_ASSERT_STRING_EQUAL(buf, "hello");

    /* empty string copies just the terminator */
    strcpy(buf, "");
    CU_ASSERT_EQUAL(buf[0], '\0');
}

void testSTRNCPY(void)
{
    char buf[8];

    /* shorter source: remainder is NUL-padded */
    memset(buf, 'x', sizeof(buf));
    strncpy(buf, "ab", sizeof(buf));
    CU_ASSERT_EQUAL(buf[0], 'a');
    CU_ASSERT_EQUAL(buf[1], 'b');
    CU_ASSERT_EQUAL(buf[2], '\0');
    CU_ASSERT_EQUAL(buf[7], '\0');

    /* exact length: no terminator written */
    memset(buf, 'x', sizeof(buf));
    strncpy(buf, "abcdefgh", 8);
    CU_ASSERT_EQUAL(buf[0], 'a');
    CU_ASSERT_EQUAL(buf[7], 'h');
}

/* ---- concatenation -------------------------------------------------- */

void testSTRCAT(void)
{
    char buf[16] = "foo";
    char *r = strcat(buf, "bar");
    CU_ASSERT_PTR_EQUAL(r, buf);
    CU_ASSERT_STRING_EQUAL(buf, "foobar");
}

void testSTRNCAT(void)
{
    char buf[16] = "foo";
    strncat(buf, "barbaz", 3);
    CU_ASSERT_STRING_EQUAL(buf, "foobar");   /* always NUL-terminates */

    char buf2[16] = "ab";
    strncat(buf2, "cd", 10);
    CU_ASSERT_STRING_EQUAL(buf2, "abcd");
}

/* ---- comparison ----------------------------------------------------- */

void testSTRCMP(void)
{
    CU_ASSERT_EQUAL(strcmp("abc", "abc"), 0);
    CU_ASSERT_TRUE(strcmp("abc", "abd") < 0);
    CU_ASSERT_TRUE(strcmp("abd", "abc") > 0);
    CU_ASSERT_TRUE(strcmp("ab", "abc") < 0);    /* prefix is smaller */
    CU_ASSERT_TRUE(strcmp("abc", "ab") > 0);
}

void testSTRNCMP(void)
{
    CU_ASSERT_EQUAL(strncmp("abcX", "abcY", 3), 0);
    CU_ASSERT_TRUE(strncmp("abcX", "abcY", 4) < 0);
    CU_ASSERT_EQUAL(strncmp("abc", "abc", 100), 0);
    CU_ASSERT_EQUAL(strncmp("any", "thing", 0), 0); /* n==0 compares nothing */
}

void testSTRCOLL(void)
{
    /* In the C/POSIX locale strcoll() behaves like strcmp(). */
    CU_ASSERT_EQUAL(strcoll("abc", "abc"), 0);
    CU_ASSERT_TRUE(strcoll("abc", "abd") < 0);
    CU_ASSERT_TRUE(strcoll("abd", "abc") > 0);
}

/* ---- length & search ------------------------------------------------ */

void testSTRLEN(void)
{
    CU_ASSERT_EQUAL(strlen(""), 0);
    CU_ASSERT_EQUAL(strlen("hello"), 5);
    CU_ASSERT_EQUAL(strlen("a\0b"), 1);
}

void testSTRCHR(void)
{
    const char *s = "hello";
    CU_ASSERT_PTR_EQUAL(strchr(s, 'e'), s + 1);
    CU_ASSERT_PTR_EQUAL(strchr(s, 'l'), s + 2);   /* first match */
    CU_ASSERT_PTR_NULL(strchr(s, 'z'));
    /* the terminating NUL is part of the search space */
    CU_ASSERT_PTR_EQUAL(strchr(s, '\0'), s + 5);
}

void testSTRRCHR(void)
{
    const char *s = "hello";
    CU_ASSERT_PTR_EQUAL(strrchr(s, 'l'), s + 3);  /* last match */
    CU_ASSERT_PTR_EQUAL(strrchr(s, 'h'), s);
    CU_ASSERT_PTR_NULL(strrchr(s, 'z'));
    CU_ASSERT_PTR_EQUAL(strrchr(s, '\0'), s + 5);
}

void testSTRSTR(void)
{
    const char *s = "the quick brown fox";
    CU_ASSERT_PTR_EQUAL(strstr(s, "quick"), s + 4);
    CU_ASSERT_PTR_EQUAL(strstr(s, ""), s);        /* empty needle */
    CU_ASSERT_PTR_NULL(strstr(s, "lazy"));
}

void testSTRSPN(void)
{
    CU_ASSERT_EQUAL(strspn("123abc", "0123456789"), 3);
    CU_ASSERT_EQUAL(strspn("abc", "0123456789"), 0);
    CU_ASSERT_EQUAL(strspn("", "abc"), 0);
}

void testSTRCSPN(void)
{
    CU_ASSERT_EQUAL(strcspn("abc123", "0123456789"), 3);
    CU_ASSERT_EQUAL(strcspn("123", "0123456789"), 0);
    CU_ASSERT_EQUAL(strcspn("abc", "xyz"), 3);
}

void testSTRPBRK(void)
{
    const char *s = "hello world";
    CU_ASSERT_PTR_EQUAL(strpbrk(s, "ol"), s + 2);   /* first 'l' */
    CU_ASSERT_PTR_NULL(strpbrk(s, "xyz"));
}

void testSTRTOK(void)
{
    char buf[] = "a,bb,,ccc";
    char *t;

    t = strtok(buf, ",");
    CU_ASSERT_PTR_NOT_NULL_FATAL(t);
    CU_ASSERT_STRING_EQUAL(t, "a");

    t = strtok(NULL, ",");
    CU_ASSERT_PTR_NOT_NULL_FATAL(t);
    CU_ASSERT_STRING_EQUAL(t, "bb");

    t = strtok(NULL, ",");          /* leading delimiters are skipped */
    CU_ASSERT_PTR_NOT_NULL_FATAL(t);
    CU_ASSERT_STRING_EQUAL(t, "ccc");

    t = strtok(NULL, ",");
    CU_ASSERT_PTR_NULL(t);
}

/* ---- mem* block functions ------------------------------------------- */

void testMEMCPY(void)
{
    char dst[8] = {0};
    char *r = memcpy(dst, "abcd", 4);
    CU_ASSERT_PTR_EQUAL(r, dst);
    CU_ASSERT_EQUAL(memcmp(dst, "abcd", 4), 0);
}

void testMEMMOVE(void)
{
    char buf[] = "abcdef";
    /* overlapping forward move */
    memmove(buf + 2, buf, 4);
    CU_ASSERT_EQUAL(memcmp(buf, "ababcd", 6), 0);

    char buf2[] = "abcdef";
    /* overlapping backward move */
    memmove(buf2, buf2 + 2, 4);
    CU_ASSERT_EQUAL(memcmp(buf2, "cdefef", 6), 0);
}

void testMEMCMP(void)
{
    CU_ASSERT_EQUAL(memcmp("abc", "abc", 3), 0);
    CU_ASSERT_TRUE(memcmp("abc", "abd", 3) < 0);
    CU_ASSERT_TRUE(memcmp("abd", "abc", 3) > 0);
    CU_ASSERT_EQUAL(memcmp("aXc", "aYc", 1), 0);  /* only first byte */
}

void testMEMCHR(void)
{
    const char *s = "hello";
    CU_ASSERT_PTR_EQUAL(memchr(s, 'l', 5), s + 2);
    CU_ASSERT_PTR_NULL(memchr(s, 'z', 5));
    CU_ASSERT_PTR_NULL(memchr(s, 'o', 3));        /* bounded by length */
}

void testMEMSET(void)
{
    char buf[8];
    char *r = memset(buf, 'A', sizeof(buf));
    CU_ASSERT_PTR_EQUAL(r, buf);
    for (int i = 0; i < 8; i++)
        CU_ASSERT_EQUAL(buf[i], 'A');

    memset(buf, 0, sizeof(buf));
    CU_ASSERT_EQUAL(buf[0], 0);
    CU_ASSERT_EQUAL(buf[7], 0);
}

/* ---- diagnostics ---------------------------------------------------- */

void testSTRERROR(void)
{
    /* strerror() must return a non-NULL, non-empty string for a known errno. */
    char *m = strerror(0);
    CU_ASSERT_PTR_NOT_NULL(m);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("string_C89_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "strcpy()", testSTRCPY)) ||
        (NULL == CU_add_test(pSuite, "strncpy()", testSTRNCPY)) ||
        (NULL == CU_add_test(pSuite, "strcat()", testSTRCAT)) ||
        (NULL == CU_add_test(pSuite, "strncat()", testSTRNCAT)) ||
        (NULL == CU_add_test(pSuite, "strcmp()", testSTRCMP)) ||
        (NULL == CU_add_test(pSuite, "strncmp()", testSTRNCMP)) ||
        (NULL == CU_add_test(pSuite, "strcoll()", testSTRCOLL)) ||
        (NULL == CU_add_test(pSuite, "strlen()", testSTRLEN)) ||
        (NULL == CU_add_test(pSuite, "strchr()", testSTRCHR)) ||
        (NULL == CU_add_test(pSuite, "strrchr()", testSTRRCHR)) ||
        (NULL == CU_add_test(pSuite, "strstr()", testSTRSTR)) ||
        (NULL == CU_add_test(pSuite, "strspn()", testSTRSPN)) ||
        (NULL == CU_add_test(pSuite, "strcspn()", testSTRCSPN)) ||
        (NULL == CU_add_test(pSuite, "strpbrk()", testSTRPBRK)) ||
        (NULL == CU_add_test(pSuite, "strtok()", testSTRTOK)) ||
        (NULL == CU_add_test(pSuite, "memcpy()", testMEMCPY)) ||
        (NULL == CU_add_test(pSuite, "memmove()", testMEMMOVE)) ||
        (NULL == CU_add_test(pSuite, "memcmp()", testMEMCMP)) ||
        (NULL == CU_add_test(pSuite, "memchr()", testMEMCHR)) ||
        (NULL == CU_add_test(pSuite, "memset()", testMEMSET)) ||
        (NULL == CU_add_test(pSuite, "strerror()", testSTRERROR)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-String-C89");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

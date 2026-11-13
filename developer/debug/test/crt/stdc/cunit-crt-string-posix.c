/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the POSIX / BSD <string.h> extensions provided by the
    AROS C runtime: strdup, strndup, strnlen, stpcpy, strcasecmp,
    strncasecmp, strlcpy, strlcat and strsep.

    These are not part of ISO C; they are exposed through POSIX / _GNU_SOURCE
    (or as historical BSD extensions).
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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

void testSTRDUP(void)
{
    char *d = strdup("hello");
    CU_ASSERT_PTR_NOT_NULL(d);
    if (d)
    {
        CU_ASSERT_STRING_EQUAL(d, "hello");
        free(d);
    }

    char *e = strdup("");
    CU_ASSERT_PTR_NOT_NULL(e);
    if (e)
    {
        CU_ASSERT_EQUAL(e[0], '\0');
        free(e);
    }
}

void testSTRNDUP(void)
{
    /* copies at most n bytes and always terminates */
    char *d = strndup("hello world", 5);
    CU_ASSERT_PTR_NOT_NULL(d);
    if (d)
    {
        CU_ASSERT_STRING_EQUAL(d, "hello");
        free(d);
    }

    /* n longer than the string just duplicates it */
    char *e = strndup("hi", 10);
    CU_ASSERT_PTR_NOT_NULL(e);
    if (e)
    {
        CU_ASSERT_STRING_EQUAL(e, "hi");
        free(e);
    }
}

void testSTRNLEN(void)
{
    CU_ASSERT_EQUAL(strnlen("hello", 10), 5);
    CU_ASSERT_EQUAL(strnlen("hello", 3), 3);   /* bounded by n */
    CU_ASSERT_EQUAL(strnlen("hello", 5), 5);
    CU_ASSERT_EQUAL(strnlen("", 10), 0);
    CU_ASSERT_EQUAL(strnlen("abc", 0), 0);
}

void testSTPCPY(void)
{
    char buf[16];
    /* stpcpy() returns a pointer to the terminating NUL of the result */
    char *end = stpcpy(buf, "hello");
    CU_ASSERT_STRING_EQUAL(buf, "hello");
    CU_ASSERT_PTR_EQUAL(end, buf + 5);
    CU_ASSERT_EQUAL(*end, '\0');

    /* chaining with stpcpy builds a string efficiently */
    char buf2[16];
    char *p = stpcpy(buf2, "foo");
    p = stpcpy(p, "bar");
    CU_ASSERT_STRING_EQUAL(buf2, "foobar");
    CU_ASSERT_PTR_EQUAL(p, buf2 + 6);
}

void testSTRCASECMP(void)
{
    CU_ASSERT_EQUAL(strcasecmp("Hello", "hello"), 0);
    CU_ASSERT_EQUAL(strcasecmp("ABC", "abc"), 0);
    CU_ASSERT_TRUE(strcasecmp("abc", "abd") < 0);
    CU_ASSERT_TRUE(strcasecmp("ABD", "abc") > 0);
}

void testSTRNCASECMP(void)
{
    CU_ASSERT_EQUAL(strncasecmp("HelloX", "helloY", 5), 0);
    CU_ASSERT_TRUE(strncasecmp("HelloX", "helloY", 6) < 0);
    CU_ASSERT_EQUAL(strncasecmp("abc", "ABC", 0), 0);
}

void testSTRLCPY(void)
{
    char buf[8];

    /* returns the length of the source it tried to create */
    size_t r = strlcpy(buf, "hello", sizeof(buf));
    CU_ASSERT_EQUAL(r, 5);
    CU_ASSERT_STRING_EQUAL(buf, "hello");

    /* truncation: result is always NUL-terminated, return is src length */
    r = strlcpy(buf, "abcdefghij", sizeof(buf));
    CU_ASSERT_EQUAL(r, 10);
    CU_ASSERT_EQUAL(buf[7], '\0');
    CU_ASSERT_STRING_EQUAL(buf, "abcdefg");
}

void testSTRLCAT(void)
{
    char buf[8] = "ab";

    size_t r = strlcat(buf, "cd", sizeof(buf));
    CU_ASSERT_EQUAL(r, 4);
    CU_ASSERT_STRING_EQUAL(buf, "abcd");

    /* truncating append: return is the length it tried to create */
    r = strlcat(buf, "efghij", sizeof(buf));
    CU_ASSERT_EQUAL(r, 10);
    CU_ASSERT_EQUAL(buf[7], '\0');
    CU_ASSERT_STRING_EQUAL(buf, "abcdefg");
}

void testSTRSEP(void)
{
    char src[] = "a,bb,,ccc";
    char *p = src;
    char *t;

    t = strsep(&p, ",");
    CU_ASSERT_PTR_NOT_NULL_FATAL(t);
    CU_ASSERT_STRING_EQUAL(t, "a");

    t = strsep(&p, ",");
    CU_ASSERT_PTR_NOT_NULL_FATAL(t);
    CU_ASSERT_STRING_EQUAL(t, "bb");

    /* unlike strtok(), strsep() returns an empty field for adjacent delims */
    t = strsep(&p, ",");
    CU_ASSERT_PTR_NOT_NULL_FATAL(t);
    CU_ASSERT_EQUAL(t[0], '\0');

    t = strsep(&p, ",");
    CU_ASSERT_PTR_NOT_NULL_FATAL(t);
    CU_ASSERT_STRING_EQUAL(t, "ccc");

    t = strsep(&p, ",");
    CU_ASSERT_PTR_NULL(t);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("string_POSIX_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "strdup()", testSTRDUP)) ||
        (NULL == CU_add_test(pSuite, "strndup()", testSTRNDUP)) ||
        (NULL == CU_add_test(pSuite, "strnlen()", testSTRNLEN)) ||
        (NULL == CU_add_test(pSuite, "stpcpy()", testSTPCPY)) ||
        (NULL == CU_add_test(pSuite, "strcasecmp()", testSTRCASECMP)) ||
        (NULL == CU_add_test(pSuite, "strncasecmp()", testSTRNCASECMP)) ||
        (NULL == CU_add_test(pSuite, "strlcpy()", testSTRLCPY)) ||
        (NULL == CU_add_test(pSuite, "strlcat()", testSTRLCAT)) ||
        (NULL == CU_add_test(pSuite, "strsep()", testSTRSEP)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-String-POSIX");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

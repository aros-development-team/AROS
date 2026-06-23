/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the C89 (ISO/IEC 9899:1990) <stdlib.h> functions:
    integer conversion and arithmetic, searching/sorting and the
    malloc()/calloc()/realloc()/free() memory management family.

    The memory tests also cover the standard-mandated edge cases:
      - calloc() must fail (return NULL) on a count*size multiplication
        overflow rather than under-allocating,
      - realloc(ptr, 0) frees ptr.
*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

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

/* ---- integer conversion --------------------------------------------- */

void testATOI(void)
{
    CU_ASSERT_EQUAL(atoi("123"), 123);
    CU_ASSERT_EQUAL(atoi("-45"), -45);
    CU_ASSERT_EQUAL(atoi("  7x"), 7);
    CU_ASSERT_EQUAL(atoi("abc"), 0);
}

void testATOL(void)
{
    CU_ASSERT_EQUAL(atol("100000"), 100000L);
    CU_ASSERT_EQUAL(atol("-9"), -9L);
}

/* ---- integer arithmetic --------------------------------------------- */

void testABS(void)
{
    CU_ASSERT_EQUAL(abs(5), 5);
    CU_ASSERT_EQUAL(abs(-5), 5);
    CU_ASSERT_EQUAL(abs(0), 0);
}

void testLABS(void)
{
    CU_ASSERT_EQUAL(labs(123456L), 123456L);
    CU_ASSERT_EQUAL(labs(-123456L), 123456L);
}

void testDIV(void)
{
    div_t d = div(17, 5);
    CU_ASSERT_EQUAL(d.quot, 3);
    CU_ASSERT_EQUAL(d.rem, 2);

    d = div(-17, 5);
    CU_ASSERT_EQUAL(d.quot, -3);    /* truncation toward zero */
    CU_ASSERT_EQUAL(d.rem, -2);
    /* identity: quot*denom + rem == numer */
    CU_ASSERT_EQUAL(d.quot * 5 + d.rem, -17);
}

void testLDIV(void)
{
    ldiv_t d = ldiv(100000L, 7L);
    CU_ASSERT_EQUAL(d.quot * 7L + d.rem, 100000L);
}

/* ---- search & sort -------------------------------------------------- */

static int int_cmp(const void *a, const void *b)
{
    int ia = *(const int *)a, ib = *(const int *)b;
    return (ia > ib) - (ia < ib);
}

void testQSORT(void)
{
    int arr[] = { 5, 3, 8, 1, 9, 2, 7 };
    const int n = sizeof(arr) / sizeof(arr[0]);

    qsort(arr, n, sizeof(int), int_cmp);

    for (int i = 1; i < n; i++)
        CU_ASSERT_TRUE(arr[i - 1] <= arr[i]);
    CU_ASSERT_EQUAL(arr[0], 1);
    CU_ASSERT_EQUAL(arr[n - 1], 9);
}

void testBSEARCH(void)
{
    int arr[] = { 1, 3, 5, 7, 9, 11 };
    const int n = sizeof(arr) / sizeof(arr[0]);
    int key;
    int *r;

    key = 7;
    r = bsearch(&key, arr, n, sizeof(int), int_cmp);
    CU_ASSERT_PTR_NOT_NULL(r);
    if (r) CU_ASSERT_EQUAL(*r, 7);

    key = 4;     /* absent */
    r = bsearch(&key, arr, n, sizeof(int), int_cmp);
    CU_ASSERT_PTR_NULL(r);
}

/* ---- memory management ---------------------------------------------- */

void testMALLOCFREE(void)
{
    void *p = malloc(64);
    CU_ASSERT_PTR_NOT_NULL(p);
    if (p)
    {
        memset(p, 0xAB, 64);   /* must be writable */
        free(p);
    }

    /* free(NULL) is a no-op */
    free(NULL);
    CU_PASS("free(NULL) did not crash");
}

void testCALLOC(void)
{
    int *p = calloc(16, sizeof(int));
    CU_ASSERT_PTR_NOT_NULL(p);
    if (p)
    {
        /* memory must be zero-initialised */
        for (int i = 0; i < 16; i++)
            CU_ASSERT_EQUAL(p[i], 0);
        free(p);
    }
}

/* calloc() must reject a count*size overflow instead of under-allocating. */
void testCALLOC_overflow(void)
{
    void *p;

    /* SIZE_MAX/2 * 4 overflows size_t. */
    errno = 0;
    p = calloc(SIZE_MAX / 2, 4);
    CU_ASSERT_PTR_NULL(p);
    if (p) free(p);

    errno = 0;
    p = calloc(SIZE_MAX, 2);
    CU_ASSERT_PTR_NULL(p);
    if (p) free(p);

    /* A zero count is always valid (and never overflows). */
    p = calloc(0, 100);
    if (p) free(p);
    CU_PASS("calloc(0, n) handled");
}

void testREALLOC_grow(void)
{
    char *p = malloc(8);
    CU_ASSERT_PTR_NOT_NULL(p);
    if (!p) return;

    memcpy(p, "1234567", 8);
    p = realloc(p, 64);
    CU_ASSERT_PTR_NOT_NULL(p);
    if (p)
    {
        /* contents up to the old size are preserved */
        CU_ASSERT_EQUAL(memcmp(p, "1234567", 8), 0);
        free(p);
    }
}

void testREALLOC_null(void)
{
    /* realloc(NULL, n) behaves like malloc(n). */
    void *p = realloc(NULL, 32);
    CU_ASSERT_PTR_NOT_NULL(p);
    if (p) free(p);
}

void testREALLOC_zero(void)
{
    /* realloc(ptr, 0) frees ptr and returns NULL in this implementation. */
    void *p = malloc(16);
    CU_ASSERT_PTR_NOT_NULL(p);
    void *r = realloc(p, 0);
    CU_ASSERT_PTR_NULL(r);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("stdlib_C89_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "atoi()", testATOI)) ||
        (NULL == CU_add_test(pSuite, "atol()", testATOL)) ||
        (NULL == CU_add_test(pSuite, "abs()", testABS)) ||
        (NULL == CU_add_test(pSuite, "labs()", testLABS)) ||
        (NULL == CU_add_test(pSuite, "div()", testDIV)) ||
        (NULL == CU_add_test(pSuite, "ldiv()", testLDIV)) ||
        (NULL == CU_add_test(pSuite, "qsort()", testQSORT)) ||
        (NULL == CU_add_test(pSuite, "bsearch()", testBSEARCH)) ||
        (NULL == CU_add_test(pSuite, "malloc()/free()", testMALLOCFREE)) ||
        (NULL == CU_add_test(pSuite, "calloc()", testCALLOC)) ||
        (NULL == CU_add_test(pSuite, "calloc() overflow guard", testCALLOC_overflow)) ||
        (NULL == CU_add_test(pSuite, "realloc() grow", testREALLOC_grow)) ||
        (NULL == CU_add_test(pSuite, "realloc(NULL, n)", testREALLOC_null)) ||
        (NULL == CU_add_test(pSuite, "realloc(ptr, 0)", testREALLOC_zero)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-Stdlib-C89");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

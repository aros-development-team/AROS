/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the BSD / GNU <stdlib.h> extensions provided by the AROS
    C runtime (stdc.library): qsort_r().

    These are not part of ISO C up to C17; qsort_r() is a historical BSD /
    glibc extension (standardised only in POSIX.1-2024).  AROS exposes it with
    the GNU/glibc argument order: the context pointer is the last argument of
    both qsort_r() and the comparison function (compar(a, b, arg)).
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
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

/* Comparator that sorts ascending or descending depending on the sign of the
   integer pointed at by arg.  Proves the context pointer is threaded through
   the recursion unchanged. */
static int cmp_dir(const void *a, const void *b, void *arg)
{
    int x = *(const int *)a;
    int y = *(const int *)b;
    int dir = *(const int *)arg;

    return dir >= 0 ? (x - y) : (y - x);
}

/* Comparator that orders elements by their distance from a pivot supplied via
   arg (the documented qsort_r() example). */
static int cmp_rel(const void *a, const void *b, void *arg)
{
    int pivot = *(const int *)arg;
    int da = abs(*(const int *)a - pivot);
    int db = abs(*(const int *)b - pivot);

    return da - db;
}

/* Comparator that records how many times it was invoked through arg, so we can
   confirm qsort_r() actually calls it (and passes our context). */
static int cmp_count(const void *a, const void *b, void *arg)
{
    (*(int *)arg)++;
    return *(const int *)a - *(const int *)b;
}

void testQSORT_R_ascending(void)
{
    int arr[] = { 5, 1, 4, 2, 8, 0, 9, 3, 7, 6 };
    int dir = 1;
    int i;

    qsort_r(arr, sizeof(arr) / sizeof(arr[0]), sizeof(int), cmp_dir, &dir);

    for (i = 0; i < 10; i++)
        CU_ASSERT_EQUAL(arr[i], i);
}

void testQSORT_R_descending(void)
{
    int arr[] = { 5, 1, 4, 2, 8, 0, 9, 3, 7, 6 };
    int dir = -1;
    int i;

    qsort_r(arr, sizeof(arr) / sizeof(arr[0]), sizeof(int), cmp_dir, &dir);

    for (i = 0; i < 10; i++)
        CU_ASSERT_EQUAL(arr[i], 9 - i);
}

void testQSORT_R_context(void)
{
    int arr[] = { 1, 9, 3, 7, 5, 4, 6, 2, 8, 0 };
    const size_t n = sizeof(arr) / sizeof(arr[0]);
    int pivot = 5;
    size_t i;

    qsort_r(arr, n, sizeof(int), cmp_rel, &pivot);

    /* The element nearest the pivot must come first, and the distance from the
       pivot must be non-decreasing across the whole array. */
    CU_ASSERT_EQUAL(arr[0], pivot);
    for (i = 1; i < n; i++)
        CU_ASSERT(abs(arr[i] - pivot) >= abs(arr[i - 1] - pivot));
}

void testQSORT_R_invoked(void)
{
    int arr[] = { 3, 1, 2 };
    int calls = 0;

    qsort_r(arr, 3, sizeof(int), cmp_count, &calls);

    CU_ASSERT(calls > 0);
    CU_ASSERT_EQUAL(arr[0], 1);
    CU_ASSERT_EQUAL(arr[1], 2);
    CU_ASSERT_EQUAL(arr[2], 3);
}

void testQSORT_R_edge(void)
{
    int empty[1] = { 42 };
    int one[1] = { 7 };
    int dir = 1;

    /* n == 0 must not touch the array nor crash. */
    qsort_r(empty, 0, sizeof(int), cmp_dir, &dir);
    CU_ASSERT_EQUAL(empty[0], 42);

    /* n == 1 is already sorted. */
    qsort_r(one, 1, sizeof(int), cmp_dir, &dir);
    CU_ASSERT_EQUAL(one[0], 7);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("stdlib_EXT_Suite", init_suite, clean_suite);
    if (NULL == pSuite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "qsort_r() ascending", testQSORT_R_ascending)) ||
        (NULL == CU_add_test(pSuite, "qsort_r() descending", testQSORT_R_descending)) ||
        (NULL == CU_add_test(pSuite, "qsort_r() context order", testQSORT_R_context)) ||
        (NULL == CU_add_test(pSuite, "qsort_r() comparator invoked", testQSORT_R_invoked)) ||
        (NULL == CU_add_test(pSuite, "qsort_r() edge cases", testQSORT_R_edge)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-Stdlib-EXT");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

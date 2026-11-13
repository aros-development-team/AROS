/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the C11 (ISO/IEC 9899:2011) <stdlib.h> function
    aligned_alloc().

    These verify that the returned storage actually meets the requested
    alignment, that the block is usable and freeable with free(), and that
    invalid alignments (zero / not a power of two / not a multiple of
    sizeof(void *)) are rejected.
*/

#include <stdio.h>
#include <stdlib.h>
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

void testALIGNED_ALLOC_basic(void)
{
    size_t aligns[] = { sizeof(void *), 16, 32, 64, 128, 256 };

    for (unsigned i = 0; i < sizeof(aligns) / sizeof(aligns[0]); i++)
    {
        size_t a = aligns[i];
        void *p = aligned_alloc(a, a * 4);
        CU_ASSERT_PTR_NOT_NULL(p);
        if (p)
        {
            /* the returned address must be aligned to 'a' */
            CU_ASSERT_EQUAL(((uintptr_t)p) % a, 0);
            /* and the storage must be usable */
            memset(p, 0x5A, a * 4);
            free(p);
        }
    }
}

void testALIGNED_ALLOC_usable(void)
{
    /* round-trip some data through an aligned block */
    int *p = aligned_alloc(64, 64 * sizeof(int));
    CU_ASSERT_PTR_NOT_NULL(p);
    if (p)
    {
        CU_ASSERT_EQUAL(((uintptr_t)p) % 64, 0);
        for (int i = 0; i < 64; i++)
            p[i] = i;
        for (int i = 0; i < 64; i++)
            CU_ASSERT_EQUAL(p[i], i);
        free(p);
    }
}

void testALIGNED_ALLOC_invalid(void)
{
    void *p;

    /* zero alignment is invalid */
    p = aligned_alloc(0, 16);
    CU_ASSERT_PTR_NULL(p);
    if (p) free(p);

    /* alignment not a power of two is invalid */
    p = aligned_alloc(24, 48);
    CU_ASSERT_PTR_NULL(p);
    if (p) free(p);

    p = aligned_alloc(3, 12);
    CU_ASSERT_PTR_NULL(p);
    if (p) free(p);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("stdlib_C11_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "aligned_alloc() alignment", testALIGNED_ALLOC_basic)) ||
        (NULL == CU_add_test(pSuite, "aligned_alloc() usable", testALIGNED_ALLOC_usable)) ||
        (NULL == CU_add_test(pSuite, "aligned_alloc() invalid args", testALIGNED_ALLOC_invalid)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-Stdlib-C11");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

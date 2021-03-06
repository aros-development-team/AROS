/*
    Copyright (C) 2021, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>

#include <stdio.h>
#include <assert.h>

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

void testBYTE(void)
{
    CU_ASSERT(1 == sizeof(BYTE));
    CU_ASSERT(1 == sizeof(UBYTE));
}

void testWORD(void)
{
    CU_ASSERT(2 == sizeof(WORD));
    CU_ASSERT(2 == sizeof(UWORD));
}

void testLONG(void)
{
    CU_ASSERT(4 == sizeof(LONG));
    CU_ASSERT(4 == sizeof(ULONG));
}

void testQUAD(void)
{
    CU_ASSERT(8 == sizeof(QUAD));
    CU_ASSERT(8 == sizeof(UQUAD));
}

void testAPTR(void)
{
#if (__WORDSIZE == 64)
    CU_ASSERT(8 == sizeof(APTR));
#else
    CU_ASSERT(4 == sizeof(APTR));
#endif
}

void testIPTR(void)
{
#if (__WORDSIZE == 64)
    CU_ASSERT(8 == sizeof(IPTR));
#else
    CU_ASSERT(4 == sizeof(IPTR));
#endif
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

   /* add a suite to the registry */
    pSuite = CU_add_suite("BasicTypes_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

   /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "test of BYTE type", testBYTE)) ||
        (NULL == CU_add_test(pSuite, "test of WORD type", testWORD)) ||
        (NULL == CU_add_test(pSuite, "test of LONG type", testLONG)) ||
        (NULL == CU_add_test(pSuite, "test of QUAD type", testQUAD)) ||
        (NULL == CU_add_test(pSuite, "test of APTR long type", testAPTR)) ||
        (NULL == CU_add_test(pSuite, "test of IPTR type", testIPTR)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("ExecUnitTests");
    CU_set_output_filename("Exec-Types");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

/*
    Copyright (C) 2021, The AROS Development Team. All rights reserved.
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/pertask.h>
#include <proto/userel.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#include "pertaskvalue.h"

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

void testADD(void)
{
    ULONG vec[3] = {1, 2, 0};
    vec[2] = DummyAdd(vec[0], vec[1]);
    if (vec[2] == vec[0] + vec[1])
    {
        CU_PASS("");
    }
    else
    {
        CU_FAIL("3 != DummyAdd(1, 2)");
    }
}

void testGPBSE(void)
{
    void *parent;
    parent = PertaskGetParentBase();
    if (parent != NULL)
    {
        CU_PASS("");
    }
    else
    {
        CU_FAIL("NULL == pertask.library->GetParentBase() (via userel.library relbase)");
    }
}

void testPTVL(void)
{
    pertaskvalue = 1;
    CU_ASSERT(1 == PertaskGetValue());
    CU_ASSERT(1 == GetChildValue());

    PertaskSetValue(2);
    CU_ASSERT(2 == GetChildValue());
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

   /* add a suite to the registry */
    pSuite = CU_add_suite("Relbase_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

   /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "test of calling dummy.library->add() via userel.library, using relbase", testADD)) ||
        (NULL == CU_add_test(pSuite, "test of calling pertask.library->getparentbase() via userel.library, using relbase", testGPBSE)) ||
        (NULL == CU_add_test(pSuite, "test of accessing pertaskbase->value via userel.library, using relbase", testPTVL)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("GenmoduleUnitTests");
    CU_set_output_filename("Genmodule-Relbase");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

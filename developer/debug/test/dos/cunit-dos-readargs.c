/*
    Copyright (C) 2021, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <stdlib.h>
#include <assert.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

enum
{
    ARG_COL,
    ARG_CNT
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


/* test of ReadArgs() with a /N parameter and a number.
 */
void testREADARGSNUMBER(void)
{
    IPTR args[ARG_CNT];
    struct RDArgs *rdargs;
    LONG colno = -1;
    
    STRPTR template = "COL=C/N";
    STRPTR param = "COL=3";
    
    if ((rdargs = AllocDosObject(DOS_RDARGS, NULL)))
    {
        rdargs->RDA_Source.CS_Buffer = param;
        rdargs->RDA_Source.CS_Length = strlen(param);
        rdargs->RDA_Source.CS_CurChr = 0;
        rdargs->RDA_DAList = 0;
        rdargs->RDA_Buffer = NULL;
        rdargs->RDA_BufSiz = 0;
        rdargs->RDA_ExtHelp = NULL;
        rdargs->RDA_Flags = 0;

        memset(args, 0, sizeof args);

        if ((ReadArgs(template, args, rdargs)))
        {
            if (args[ARG_COL])
                colno = *(LONG *) args[ARG_COL];

            CU_ASSERT(3 == colno);
            
            FreeArgs(rdargs);
        }
        else
        {
            CU_FAIL("ReadArgs() returned NULL");
        }

        FreeDosObject(DOS_RDARGS, rdargs);
    }
    else
    {
        CU_FAIL("AllocDosObject() returned NULL");
    }
}


/* test of ReadArgs() with a /N parameter and a number
 * followed by a space.
 */
void testREADARGSNUMBERSPACE(void)
{
    IPTR args[ARG_CNT];
    struct RDArgs *rdargs;
    LONG colno = -1;
    
    STRPTR template = "COL=C/N";
    STRPTR param = "COL=3 ";
    
    if ((rdargs = AllocDosObject(DOS_RDARGS, NULL)))
    {
        rdargs->RDA_Source.CS_Buffer = param;
        rdargs->RDA_Source.CS_Length = strlen(param);
        rdargs->RDA_Source.CS_CurChr = 0;
        rdargs->RDA_DAList = 0;
        rdargs->RDA_Buffer = NULL;
        rdargs->RDA_BufSiz = 0;
        rdargs->RDA_ExtHelp = NULL;
        rdargs->RDA_Flags = 0;

        memset(args, 0, sizeof args);

        if ((ReadArgs(template, args, rdargs)))
        {
            if (args[ARG_COL])
                colno = *(LONG *) args[ARG_COL];

            CU_ASSERT(3 == colno);
            
            FreeArgs(rdargs);
        }
        else
        {
            CU_FAIL("ReadArgs() returned NULL");
        }

        FreeDosObject(DOS_RDARGS, rdargs);
    }
    else
    {
        CU_FAIL("AllocDosObject() returned NULL");
    }
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

   /* add a suite to the registry */
    pSuite = CU_add_suite("ReadArgs_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

   /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "test of ReadArgs() /N number", testREADARGSNUMBER)) ||
        (NULL == CU_add_test(pSuite, "test of ReadArgs() /N number with space", testREADARGSNUMBERSPACE)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("DOSUnitTests");
    CU_set_output_filename("DOS-ReadArgs");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

/*
    Copyright © 2021, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/locale.h>
#include <string.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#include <aros/debug.h>

/* storage used during testing */
static char buffer[100];

#pragma pack(2)
struct Args
{
    STRPTR  arg1;
    STRPTR  arg2;
    UWORD   arg3;
    UWORD   arg4;
    void *  arg5; /* unused */
};
#pragma pack()

static struct Args args = {"ARG1", "ARG2", 'N', 50, NULL};
static struct Hook hook;

AROS_UFH3(VOID, LocRawDoFmtFormatStringFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct Locale *, locale, A2),
    AROS_UFHA(char, fill, A1))
{
    AROS_USERFUNC_INIT
#ifdef __mc68000__
    register char *pdata asm("a3") = hook->h_Data;
#else
    char *pdata = hook->h_Data;
#endif

    *pdata++ = fill;

    hook->h_Data = pdata;

    AROS_USERFUNC_EXIT
}

/* The suite initialization function.
  * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    hook.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(LocRawDoFmtFormatStringFunc);
    hook.h_SubEntry = 0; //RAWFMTFUNC_STRING;
    hook.h_Data = buffer;
    return 0;
}

/* The suite cleanup function.
  * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
    return 0;
}

/* Test of FormatString() without format specifier.
 */
void testFORMATSTRINGNOFORMAT(void)
{
    /*
     * When the template string doesn't contain a format specifier the return
     * value of FormatString() must point to the 1st argument so that the next
     * call of FormatString() can fetch that argument.
     */
    CU_ASSERT(&args.arg1 == FormatString(NULL, (STRPTR)"Textformat", (RAWARG)&args, &hook));
    CU_ASSERT(0 == strcmp("Textformat", buffer));
}

/* Test of FormatString() with a single str format specifier.
 */
void testFORMATSTRINGSINGLESTRFMTARG(void)
{
    hook.h_Data = buffer;
    /*
     * When the template string contains one format specifier the return value of
     * FormatString() must point to the 2nd argument so that the next call of
     * FormatString() can fetch that argument.
     */
    CU_ASSERT(&args.arg2 == FormatString(NULL, (STRPTR)"Textformat %s", (RAWARG)&args, &hook));
    CU_ASSERT(0 == strcmp("Textformat ARG1", buffer));
}

/* Test of FormatString() with a single char format specifier.
 */
void testFORMATSTRINGSINGLECHRFMTARG(void)
{
    hook.h_Data = buffer;

    CU_ASSERT(&args.arg4 == FormatString(NULL, (STRPTR)"Textformat %c", (RAWARG)&args.arg3, &hook));
    CU_ASSERT(0 == strcmp("Textformat N", buffer));
}

/* Test of FormatString() with a format containing multiple specifiers.
 */
void testFORMATSTRINGFMTMULTIARG(void)
{
    hook.h_Data = buffer;

    CU_ASSERT(&args.arg5 == FormatString(NULL, (STRPTR)"Textformat %s %s %c %u", (RAWARG)&args, &hook));
    CU_ASSERT(0 == strcmp("Textformat ARG1 ARG2 N 50", buffer));
}

/* Test of FormatString() with a format containing explicit argument no. specifiers.
 */
void testFORMATSTRINGFMTEXPLICITARG(void)
{
    hook.h_Data = buffer;

    CU_ASSERT(&args.arg5 == FormatString(NULL, (STRPTR)"Textformat %2$s %4$u", (RAWARG)&args, &hook));
    CU_ASSERT(0 == strcmp("Textformat ARG2 50", buffer));
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("FormatString_Suite", init_suite, clean_suite);
    if (NULL == pSuite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "test with no format specifier", testFORMATSTRINGNOFORMAT)) ||
        (NULL == CU_add_test(pSuite, "test with format containing a single string specifier", testFORMATSTRINGSINGLESTRFMTARG)) ||
        (NULL == CU_add_test(pSuite, "test with format containing a single char specifier",testFORMATSTRINGSINGLECHRFMTARG)) ||
        (NULL == CU_add_test(pSuite, "test with format containing multiple specifiers",testFORMATSTRINGFMTMULTIARG)) ||
        (NULL == CU_add_test(pSuite, "test with format containing explicit arg specifiers",testFORMATSTRINGFMTEXPLICITARG)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("LocaleFormatStringUnitTests");
    CU_set_output_filename("Locale-FormatString");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

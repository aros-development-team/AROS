/*
    Copyright © 2021, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/locale.h>
#include <string.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

/* storage used during testing */
static char buffer[100];

#pragma pack(2)
struct Args
{
    STRPTR arg1;
    STRPTR arg2;
};
#pragma pack()

static struct Args args = {"ARG1", "ARG2"};

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
    struct Hook hook;
    APTR retval;

    hook.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(LocRawDoFmtFormatStringFunc);
    hook.h_SubEntry = 0; //RAWFMTFUNC_STRING;
    hook.h_Data = buffer;

    STRPTR textformat = "Textformat";

    retval = FormatString(NULL, (STRPTR)textformat, (RAWARG)&args, &hook);

    /*
     * When the template string doesn't contain a format specifier the return
     * value of FormatString() must point to the 1st argument so that the next
     * call of FormatString() can fetch that argument.
     */
    CU_ASSERT(args.arg1 == retval);
    CU_ASSERT(strcmp("Textformat", buffer) == 0);
}


/* Test of FormatString() with format specifier.
 */
void testFORMATSTRINGFORMAT(void)
{
    struct Hook hook;
    APTR retval;

    hook.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(LocRawDoFmtFormatStringFunc);
    hook.h_SubEntry = 0; //RAWFMTFUNC_STRING;
    hook.h_Data = buffer;

    STRPTR textformat = "Textformat %s"; /* one format specifier */

    retval = FormatString(NULL, (STRPTR)textformat, (RAWARG)&args, &hook);

    /*
     * When the template string contains one format specifier the return value of
     * FormatString() must point to the 2nd argument so that the next call of
     * FormatString() can fetch that argument.
     */
    CU_ASSERT(args.arg2 == retval);
    CU_ASSERT(strcmp("Textformat ARG1", buffer) == 0);
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
    if ((NULL == CU_add_test(pSuite, "test of FormatString w/o format specifier", testFORMATSTRINGNOFORMAT)) ||
        (NULL == CU_add_test(pSuite, "test of FormatString with format specifier", testFORMATSTRINGFORMAT)))
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

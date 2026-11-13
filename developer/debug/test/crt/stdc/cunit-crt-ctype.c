/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for <ctype.h> character classification and conversion.

    Functionality is split by the C standard that exposes it:
      - the C89 classification/conversion functions, and
      - the C99 addition isblank().

    Particular attention is paid to the requirement that every is*()/to*()
    function accept EOF and any value representable as an unsigned char.
*/

#define _GNU_SOURCE     /* expose isblank() (C99; gated behind POSIX/GNU here) */
#include <stdio.h>
#include <ctype.h>

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

/* ---- C89 (ISO/IEC 9899:1990) ---------------------------------------- */

void testISDIGIT(void)
{
    CU_ASSERT_TRUE(isdigit('0'));
    CU_ASSERT_TRUE(isdigit('9'));
    CU_ASSERT_FALSE(isdigit('a'));
    CU_ASSERT_FALSE(isdigit(' '));
    CU_ASSERT_FALSE(isdigit('/'));
    CU_ASSERT_FALSE(isdigit(':'));
}

void testISXDIGIT(void)
{
    CU_ASSERT_TRUE(isxdigit('0'));
    CU_ASSERT_TRUE(isxdigit('9'));
    CU_ASSERT_TRUE(isxdigit('a'));
    CU_ASSERT_TRUE(isxdigit('F'));
    CU_ASSERT_FALSE(isxdigit('g'));
    CU_ASSERT_FALSE(isxdigit('G'));
}

void testISALPHA(void)
{
    CU_ASSERT_TRUE(isalpha('a'));
    CU_ASSERT_TRUE(isalpha('Z'));
    CU_ASSERT_FALSE(isalpha('0'));
    CU_ASSERT_FALSE(isalpha('_'));
}

void testISALNUM(void)
{
    CU_ASSERT_TRUE(isalnum('a'));
    CU_ASSERT_TRUE(isalnum('0'));
    CU_ASSERT_FALSE(isalnum('+'));
    CU_ASSERT_FALSE(isalnum(' '));
}

void testISUPPERLOWER(void)
{
    CU_ASSERT_TRUE(isupper('A'));
    CU_ASSERT_FALSE(isupper('a'));
    CU_ASSERT_TRUE(islower('z'));
    CU_ASSERT_FALSE(islower('Z'));
}

void testISSPACE(void)
{
    CU_ASSERT_TRUE(isspace(' '));
    CU_ASSERT_TRUE(isspace('\t'));
    CU_ASSERT_TRUE(isspace('\n'));
    CU_ASSERT_TRUE(isspace('\r'));
    CU_ASSERT_TRUE(isspace('\f'));
    CU_ASSERT_TRUE(isspace('\v'));
    CU_ASSERT_FALSE(isspace('a'));
}

void testISPUNCTGRAPHPRINT(void)
{
    CU_ASSERT_TRUE(ispunct('.'));
    CU_ASSERT_FALSE(ispunct('a'));
    CU_ASSERT_FALSE(ispunct(' '));

    CU_ASSERT_TRUE(isgraph('a'));
    CU_ASSERT_FALSE(isgraph(' '));   /* space is printable but not graph */

    CU_ASSERT_TRUE(isprint('a'));
    CU_ASSERT_TRUE(isprint(' '));
    CU_ASSERT_FALSE(isprint('\t'));
}

void testISCNTRL(void)
{
    CU_ASSERT_TRUE(iscntrl('\t'));
    CU_ASSERT_TRUE(iscntrl('\n'));
    CU_ASSERT_FALSE(iscntrl('a'));
    CU_ASSERT_FALSE(iscntrl(' '));
}

void testTOUPPERLOWER(void)
{
    CU_ASSERT_EQUAL(toupper('a'), 'A');
    CU_ASSERT_EQUAL(toupper('z'), 'Z');
    CU_ASSERT_EQUAL(toupper('A'), 'A');   /* idempotent on uppercase */
    CU_ASSERT_EQUAL(toupper('5'), '5');   /* non-alpha unchanged */

    CU_ASSERT_EQUAL(tolower('A'), 'a');
    CU_ASSERT_EQUAL(tolower('Z'), 'z');
    CU_ASSERT_EQUAL(tolower('a'), 'a');
    CU_ASSERT_EQUAL(tolower('5'), '5');
}

/* The classification functions must accept EOF (and only return true for
   real characters). This guards against indexing the table with EOF. */
void testEOFHANDLING(void)
{
    CU_ASSERT_FALSE(isalpha(EOF));
    CU_ASSERT_FALSE(isdigit(EOF));
    CU_ASSERT_FALSE(isalnum(EOF));
    CU_ASSERT_FALSE(isspace(EOF));
    CU_ASSERT_FALSE(isprint(EOF));
    CU_ASSERT_FALSE(isgraph(EOF));
    CU_ASSERT_FALSE(ispunct(EOF));
    CU_ASSERT_FALSE(iscntrl(EOF));
    CU_ASSERT_FALSE(isupper(EOF));
    CU_ASSERT_FALSE(islower(EOF));
    CU_ASSERT_FALSE(isxdigit(EOF));

    /* toupper()/tolower() must return EOF unchanged. */
    CU_ASSERT_EQUAL(toupper(EOF), EOF);
    CU_ASSERT_EQUAL(tolower(EOF), EOF);
}

/* All values representable as unsigned char (0..255) must be accepted. */
void testFULLUCHARRANGE(void)
{
    int c;
    for (c = 0; c <= 255; c++)
    {
        /* A character is alnum iff it is alpha or digit. */
        CU_ASSERT_EQUAL(!!isalnum(c), !!(isalpha(c) || isdigit(c)));
        /* graph implies print. */
        if (isgraph(c))
            CU_ASSERT_TRUE(isprint(c));
        /* upper/lower are alpha. */
        if (isupper(c) || islower(c))
            CU_ASSERT_TRUE(isalpha(c));
    }
}

/* ---- C99 (ISO/IEC 9899:1999) ---------------------------------------- */

void testISBLANK(void)
{
    CU_ASSERT_TRUE(isblank(' '));
    CU_ASSERT_TRUE(isblank('\t'));
    CU_ASSERT_FALSE(isblank('\n'));
    CU_ASSERT_FALSE(isblank('a'));
    CU_ASSERT_FALSE(isblank(EOF));
}

int main(void)
{
    CU_pSuite pSuiteC89 = NULL;
    CU_pSuite pSuiteC99 = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuiteC89 = CU_add_suite("ctype_C89_Suite", init_suite, clean_suite);
    if (NULL == pSuiteC89) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuiteC89, "isdigit()", testISDIGIT)) ||
        (NULL == CU_add_test(pSuiteC89, "isxdigit()", testISXDIGIT)) ||
        (NULL == CU_add_test(pSuiteC89, "isalpha()", testISALPHA)) ||
        (NULL == CU_add_test(pSuiteC89, "isalnum()", testISALNUM)) ||
        (NULL == CU_add_test(pSuiteC89, "isupper()/islower()", testISUPPERLOWER)) ||
        (NULL == CU_add_test(pSuiteC89, "isspace()", testISSPACE)) ||
        (NULL == CU_add_test(pSuiteC89, "ispunct()/isgraph()/isprint()", testISPUNCTGRAPHPRINT)) ||
        (NULL == CU_add_test(pSuiteC89, "iscntrl()", testISCNTRL)) ||
        (NULL == CU_add_test(pSuiteC89, "toupper()/tolower()", testTOUPPERLOWER)) ||
        (NULL == CU_add_test(pSuiteC89, "EOF handling", testEOFHANDLING)) ||
        (NULL == CU_add_test(pSuiteC89, "full unsigned char range", testFULLUCHARRANGE)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    pSuiteC99 = CU_add_suite("ctype_C99_Suite", init_suite, clean_suite);
    if (NULL == pSuiteC99) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuiteC99, "isblank()", testISBLANK)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-Ctype");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>

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

void test_wctrans_lookup(void) {
    wctrans_t t1 = wctrans("tolower");
    wctrans_t t2 = wctrans("toupper");
    wctrans_t invalid = wctrans("not-a-transform");

    CU_ASSERT_PTR_NOT_NULL(t1);
    CU_ASSERT_PTR_NOT_NULL(t2);
    CU_ASSERT_PTR_NULL(invalid);
}

void test_towctrans(void) {
    wctrans_t tolower_trans = wctrans("tolower");
    wctrans_t toupper_trans = wctrans("toupper");

    CU_ASSERT_EQUAL(towctrans(L'A', tolower_trans), L'a');
    CU_ASSERT_EQUAL(towctrans(L'a', toupper_trans), L'A');
    CU_ASSERT_EQUAL(towctrans(L'!', tolower_trans), L'!');  // No change
}

void test_wctype_lookup(void) {
    CU_ASSERT_PTR_NOT_NULL(wctype("alpha"));
    CU_ASSERT_PTR_NOT_NULL(wctype("digit"));
    CU_ASSERT_PTR_NULL(wctype("not-a-class"));
}

void test_iswctype(void) {
    wctype_t alpha = wctype("alpha");
    wctype_t digit = wctype("digit");

    CU_ASSERT_TRUE(iswctype(L'A', alpha));
    CU_ASSERT_TRUE(iswctype(L'z', alpha));
    CU_ASSERT_TRUE(iswctype(L'0', digit));
    CU_ASSERT_FALSE(iswctype(L'Z', digit));
}

void test_isw_functions_ascii(void) {
    CU_ASSERT_TRUE(iswalpha(L'A'));
    CU_ASSERT_TRUE(iswalpha(L'z'));
    CU_ASSERT_TRUE(iswdigit(L'3'));
    CU_ASSERT_TRUE(iswupper(L'Q'));
    CU_ASSERT_TRUE(iswlower(L'm'));
    CU_ASSERT_FALSE(iswalpha(L'!'));
    CU_ASSERT_TRUE(iswspace(L'\n'));
    CU_ASSERT_TRUE(iswblank(L'\t'));
}

void test_towupper_basic(void) {
    CU_ASSERT_EQUAL(towupper(L'a'), L'A');
    CU_ASSERT_EQUAL(towupper(L'z'), L'Z');
    CU_ASSERT_EQUAL(towupper(L'A'), L'A');  // Already upper
    CU_ASSERT_EQUAL(towupper(L'0'), L'0');  // Not letters...
    CU_ASSERT_EQUAL(towupper(L'?'), L'?');
}

void test_towlower_basic(void) {
    CU_ASSERT_EQUAL(towlower(L'A'), L'a');
    CU_ASSERT_EQUAL(towlower(L'Z'), L'z');
    CU_ASSERT_EQUAL(towlower(L'a'), L'a');  // Already lower
    CU_ASSERT_EQUAL(towlower(L'9'), L'9');  // Not letters...
    CU_ASSERT_EQUAL(towlower(L'?'), L'?');
}

void test_latin1_letters(void) {
    // U+00C0 (À) to U+00D6 (Ö) are uppercase Latin-1
    CU_ASSERT_TRUE(iswupper(0x00C0));
    CU_ASSERT_FALSE(iswlower(0x00C0));
    CU_ASSERT_TRUE(iswalpha(0x00C0));
    CU_ASSERT_EQUAL(towlower(0x00C0), 0x00E0);  // À ? à

    // U+00E9 (é) is lowercase
    CU_ASSERT_TRUE(iswlower(0x00E9));
    CU_ASSERT_TRUE(iswalpha(0x00E9));
    CU_ASSERT_EQUAL(towupper(0x00E9), 0x00C9);  // é ? É
}

void test_unicode_beyond_ascii(void) {
    // Greek capital letter Pi (U+03A0), Greek small letter pi (U+03C0)
    // This is expected to fail at the moment, since we dont
    // handle higher than latin 1 presently.
    CU_ASSERT_TRUE(iswalpha(0x03A0));
    CU_ASSERT_TRUE(iswalpha(0x03C0));
    CU_ASSERT_EQUAL(towlower(0x03A0), 0x03C0);
    CU_ASSERT_EQUAL(towupper(0x03C0), 0x03A0);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

   /* add a suite to the registry */
    pSuite = CU_add_suite("WCTYPE_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

   /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "wctrans", test_wctrans_lookup)) ||
        (NULL == CU_add_test(pSuite, "towctrans", test_towctrans)) ||
        (NULL == CU_add_test(pSuite, "wctype lookup", test_wctype_lookup)) ||
        (NULL == CU_add_test(pSuite, "iswctype generic", test_iswctype)) ||
        (NULL == CU_add_test(pSuite, "isw* basic ASCII", test_isw_functions_ascii)) ||
        (NULL == CU_add_test(pSuite, "towupper", test_towupper_basic)) ||
        (NULL == CU_add_test(pSuite, "towlower", test_towlower_basic)) ||
        (NULL == CU_add_test(pSuite, "Latin-1 classification and case", test_latin1_letters)) ||
        (NULL == CU_add_test(pSuite, "Unicode beyond ASCII", test_unicode_beyond_ascii)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-WCTYPE");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <math.h>
#include <complex.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#define EPSILON 1e-9

// Helper to compare complex numbers
int complex_equal(double complex a, double complex b) {
    return cabs(a - b) < EPSILON;
}

/* The suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite1(void)
{
    return 0;
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite1(void)
{
    return 0;
}

void test_cabs(void) {
    // Test case 1: basic Pythagorean triple
    double complex z1 = 3.0 + 4.0*I;
    double expected1 = 5.0; // sqrt(3^2 + 4^2)
    CU_ASSERT_DOUBLE_EQUAL(cabs(z1), expected1, EPSILON);

    // Test case 2: purely real number
    double complex z2 = 5.0 + 0.0*I;
    double expected2 = 5.0;
    CU_ASSERT_DOUBLE_EQUAL(cabs(z2), expected2, EPSILON);

    // Test case 3: purely imaginary number
    double complex z3 = 0.0 + 7.0*I;
    double expected3 = 7.0;
    CU_ASSERT_DOUBLE_EQUAL(cabs(z3), expected3, EPSILON);

    // Test case 4: negative real and imaginary
    double complex z4 = -6.0 - 8.0*I;
    double expected4 = 10.0; // sqrt(36 + 64)
    CU_ASSERT_DOUBLE_EQUAL(cabs(z4), expected4, EPSILON);
}

void test_cexp(void) {
    double complex z = 1.0 + 1.0*I;
    double complex expected = cexp(1.0) * (cos(1.0) + I*sin(1.0));
    CU_ASSERT_TRUE(complex_equal(cexp(z), expected));
}

void test_csin(void) {
    double complex z = 1.0 + 2.0*I;
    double complex expected = sin(1.0)*cosh(2.0) + I*cos(1.0)*sinh(2.0);
    CU_ASSERT_TRUE(complex_equal(csin(z), expected));
}

void test_ccos(void) {
    double complex z = 1.0 + 2.0*I;
    double complex expected = cos(1.0)*cosh(2.0) - I*sin(1.0)*sinh(2.0);
    CU_ASSERT_TRUE(complex_equal(ccos(z), expected));
}

void test_csqrt(void) {
    double complex z = -1.0 + 0.0*I;
    double complex expected = 0.0 + 1.0*I;
    CU_ASSERT_TRUE(complex_equal(csqrt(z), expected));
}

void test_cpow(void) {
    double complex base = 0.0 + 1.0*I;
    double complex expn = 2.0 + 0.0*I;
    double complex expected = -1.0 + 0.0*I;
    CU_ASSERT_TRUE(complex_equal(cpow(base, expn), expected));
}

int main()
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("CrtComplexMath_Suite", init_suite1, clean_suite1);
    if (NULL == pSuite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "test of cabs()",  test_cabs)) ||
        (NULL == CU_add_test(pSuite, "test of cexp()",  test_cexp))  ||
        (NULL == CU_add_test(pSuite, "test of csin()",  test_csin))  ||
        (NULL == CU_add_test(pSuite, "test of ccos()",  test_ccos))  ||
        (NULL == CU_add_test(pSuite, "test of csqrt()", test_csqrt)) ||
        (NULL == CU_add_test(pSuite, "test of cpow()",  test_cpow)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTComplexMathUnitTests");
    CU_set_output_filename("CRT-ComplexMath");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    
    return CU_get_error();
}

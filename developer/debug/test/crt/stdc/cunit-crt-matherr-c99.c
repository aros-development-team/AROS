/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the C99 math error reporting (Annex F / math_errhandling)
    of the pole, domain and range errors raised by pow(), tgamma() and
    lgamma(): a pole error raises FE_DIVBYZERO, a domain error raises
    FE_INVALID (errno EDOM), and an overflow raises FE_OVERFLOW (errno ERANGE).
*/

#include <math.h>
#include <fenv.h>
#include <errno.h>
#include <float.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

int init_suite(void) { return 0; }
int clean_suite(void) { return 0; }

static void clear(void)
{
    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);
}

/* Assert that the FE exception 'exc' and errno 'err' were reported, honouring
   whichever mechanisms math_errhandling advertises. */
static void expect_err(int exc, int err)
{
    if (math_errhandling & MATH_ERREXCEPT)
        CU_ASSERT(fetestexcept(exc) != 0);
    if (math_errhandling & MATH_ERRNO)
        CU_ASSERT_EQUAL(errno, err);
}

/* pow(0, -1): pole error -> +inf, FE_DIVBYZERO, ERANGE. */
void test_pow_pole(void)
{
    double r;

    clear();
    r = pow(0.0, -1.0);
    CU_ASSERT(isinf(r));
    expect_err(FE_DIVBYZERO, ERANGE);
}

/* pow(2, 2000): overflow -> +inf, FE_OVERFLOW, ERANGE. */
void test_pow_overflow(void)
{
    double r;

    clear();
    r = pow(2.0, 2000.0);
    CU_ASSERT(isinf(r));
    expect_err(FE_OVERFLOW, ERANGE);
}

/* pow(-1, 0.5): domain error -> NaN, FE_INVALID, EDOM. */
void test_pow_domain(void)
{
    double r;

    clear();
    r = pow(-1.0, 0.5);
    CU_ASSERT(isnan(r));
    expect_err(FE_INVALID, EDOM);
}

/* A finite, in-range call raises nothing and leaves errno clear. */
void test_pow_normal(void)
{
    double r;

    clear();
    r = pow(2.0, 10.0);
    CU_ASSERT_DOUBLE_EQUAL(r, 1024.0, 0.0001);
    if (math_errhandling & MATH_ERREXCEPT)
        CU_ASSERT(fetestexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW) == 0);
}

/* tgamma(0): pole error -> inf, FE_DIVBYZERO, ERANGE. */
void test_tgamma_pole(void)
{
    double r;

    clear();
    r = tgamma(0.0);
    CU_ASSERT(isinf(r));
    expect_err(FE_DIVBYZERO, ERANGE);
}

/* tgamma(-2): a negative integer is a domain error -> NaN, FE_INVALID, EDOM. */
void test_tgamma_domain(void)
{
    double r;

    clear();
    r = tgamma(-2.0);
    CU_ASSERT(isnan(r));
    expect_err(FE_INVALID, EDOM);
}

/* tgamma(5) == 24, no error. */
void test_tgamma_normal(void)
{
    double r;

    clear();
    r = tgamma(5.0);
    CU_ASSERT_DOUBLE_EQUAL(r, 24.0, 0.0001);
    if (math_errhandling & MATH_ERREXCEPT)
        CU_ASSERT(fetestexcept(FE_DIVBYZERO | FE_INVALID) == 0);
}

/* lgamma(0) and lgamma(-3): pole errors -> +inf, FE_DIVBYZERO, ERANGE. */
void test_lgamma_pole(void)
{
    double r;

    clear();
    r = lgamma(0.0);
    CU_ASSERT(isinf(r));
    expect_err(FE_DIVBYZERO, ERANGE);

    clear();
    r = lgamma(-3.0);
    CU_ASSERT(isinf(r));
    expect_err(FE_DIVBYZERO, ERANGE);
}

/* lgamma(1) == 0, no error. */
void test_lgamma_normal(void)
{
    double r;

    clear();
    r = lgamma(1.0);
    CU_ASSERT_DOUBLE_EQUAL(r, 0.0, 0.0001);
    if (math_errhandling & MATH_ERREXCEPT)
        CU_ASSERT(fetestexcept(FE_DIVBYZERO) == 0);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("MATHERR_C99_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "pow_pole", test_pow_pole)) ||
        (NULL == CU_add_test(pSuite, "pow_overflow", test_pow_overflow)) ||
        (NULL == CU_add_test(pSuite, "pow_domain", test_pow_domain)) ||
        (NULL == CU_add_test(pSuite, "pow_normal", test_pow_normal)) ||
        (NULL == CU_add_test(pSuite, "tgamma_pole", test_tgamma_pole)) ||
        (NULL == CU_add_test(pSuite, "tgamma_domain", test_tgamma_domain)) ||
        (NULL == CU_add_test(pSuite, "tgamma_normal", test_tgamma_normal)) ||
        (NULL == CU_add_test(pSuite, "lgamma_pole", test_lgamma_pole)) ||
        (NULL == CU_add_test(pSuite, "lgamma_normal", test_lgamma_normal)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-MATHERR-C99");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <math.h>
#include <fenv.h>
#include <errno.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#include <float.h>  // For LDBL_EPSILON

#ifndef M_PIl
#define M_PIl 3.141592653589793238462643383279502884L
#endif

#define GRANULARITY_L 1e-15L
static const long double epsilon = 1e-12L;  // Adjust precision for long double if needed

/*
 * C99 long double versions of functions
 */

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

static void assert_ldouble_equal(long double actual, long double expected, long double tol) {
    if (isnanl(expected)) {
        CU_ASSERT(isnanl(actual));
    } else if (isinf(expected)) {
        CU_ASSERT(isinf(actual) && (signbit(actual) == signbit(expected)));
    } else {
        CU_ASSERT(fabsl(actual - expected) <= tol);
    }
}

void testSINL(void)
{
    assert_ldouble_equal(sinl(0.0L), 0.0L, GRANULARITY_L);
    assert_ldouble_equal(sinl(5.0L), -0.9589242746631385L, GRANULARITY_L);
    assert_ldouble_equal(sinl(-5.0L), 0.9589242746631385L, GRANULARITY_L);
}

void testCOSL(void)
{
    assert_ldouble_equal(cosl(0.0L), 1.0L, GRANULARITY_L);
    assert_ldouble_equal(cosl(5.0L), 0.28366218546322625L, GRANULARITY_L);
    assert_ldouble_equal(cosl(-5.0L), 0.28366218546322625L, GRANULARITY_L);
}

void testTANL(void)
{
    assert_ldouble_equal(tanl(0.0L), 0.0L, GRANULARITY_L);
    assert_ldouble_equal(tanl(5.0L), -3.3805150062465856L, GRANULARITY_L);
    assert_ldouble_equal(tanl(-5.0L), 3.3805150062465856L, GRANULARITY_L);

    /* tanl(pi/2) tends to infinity, just test large value */
    CU_ASSERT(tanl(M_PIl / 2) > 1e15L);
}

void testASINL(void)
{
    assert_ldouble_equal(asinl(0.0L), 0.0L, GRANULARITY_L);
    assert_ldouble_equal(asinl(1.0L), M_PIl / 2, GRANULARITY_L);
    assert_ldouble_equal(asinl(-1.0L), -M_PIl / 2, GRANULARITY_L);
    /* Domain error for >1 or < -1 */
    errno = 0;
    long double res = asinl(2.0L);
    CU_ASSERT(isnanl(res));
    CU_ASSERT(errno == EDOM);
}

void testACOSL(void)
{
    assert_ldouble_equal(acosl(0.0L), M_PIl / 2, GRANULARITY_L);
    assert_ldouble_equal(acosl(1.0L), 0.0L, GRANULARITY_L);
    assert_ldouble_equal(acosl(-1.0L), M_PIl, GRANULARITY_L);
    /* Domain error */
    errno = 0;
    long double res = acosl(2.0L);
    CU_ASSERT(isnanl(res));
    CU_ASSERT(errno == EDOM);
}

void testATANL(void)
{
    assert_ldouble_equal(atanl(0.0L), 0.0L, GRANULARITY_L);
    assert_ldouble_equal(atanl(1.0L), M_PIl / 4, GRANULARITY_L);
    assert_ldouble_equal(atanl(-1.0L), -M_PIl / 4, GRANULARITY_L);
}

void testATAN2L(void)
{
    assert_ldouble_equal(atan2l(0.0L, 1.0L), 0.0L, GRANULARITY_L);
    assert_ldouble_equal(atan2l(1.0L, 0.0L), M_PIl / 2, GRANULARITY_L);
    assert_ldouble_equal(atan2l(-1.0L, 0.0L), -M_PIl / 2, GRANULARITY_L);
}

void testSINHL(void)
{
    assert_ldouble_equal(sinhl(0.0L), 0.0L, GRANULARITY_L);
    assert_ldouble_equal(sinhl(1.0L), 1.1752011936438014L, GRANULARITY_L);
    assert_ldouble_equal(sinhl(-1.0L), -1.1752011936438014L, GRANULARITY_L);
}

void testCOSHL(void)
{
    assert_ldouble_equal(coshl(0.0L), 1.0L, GRANULARITY_L);
    assert_ldouble_equal(coshl(1.0L), 1.5430806348152437L, GRANULARITY_L);
    assert_ldouble_equal(coshl(-1.0L), 1.5430806348152437L, GRANULARITY_L);
}

void testTANHL(void)
{
    assert_ldouble_equal(tanhl(0.0L), 0.0L, GRANULARITY_L);
    assert_ldouble_equal(tanhl(1.0L), 0.7615941559557649L, GRANULARITY_L);
    assert_ldouble_equal(tanhl(-1.0L), -0.7615941559557649L, GRANULARITY_L);
}

void testEXPL(void)
{
    assert_ldouble_equal(expl(0.0L), 1.0L, GRANULARITY_L);
    assert_ldouble_equal(expl(1.0L), 2.718281828459045L, GRANULARITY_L);
    assert_ldouble_equal(expl(-1.0L), 0.3678794411714423L, GRANULARITY_L);
}

void testLOGL(void)
{
    assert_ldouble_equal(logl(1.0L), 0.0L, GRANULARITY_L);
    assert_ldouble_equal(logl(M_E), 1.0L, GRANULARITY_L);
    /* logl(0) -> -inf, errno = ERANGE */
    errno = 0;
    long double res = logl(0.0L);
    CU_ASSERT(isinf(res) && res < 0);
    CU_ASSERT(errno == ERANGE);
}

void testLOG10L(void)
{
    assert_ldouble_equal(log10l(1.0L), 0.0L, GRANULARITY_L);
    assert_ldouble_equal(log10l(10.0L), 1.0L, GRANULARITY_L);
}

void testSQRTL(void)
{
    assert_ldouble_equal(sqrtl(0.0L), 0.0L, GRANULARITY_L);
    assert_ldouble_equal(sqrtl(4.0L), 2.0L, GRANULARITY_L);
    assert_ldouble_equal(sqrtl(2.0L), 1.4142135623730951L, GRANULARITY_L);
    /* sqrtl(-1) -> NaN, errno=EDOM */
    errno = 0;
    long double res = sqrtl(-1.0L);
    CU_ASSERT(isnanl(res));
    CU_ASSERT(errno == EDOM);
}

void testCEILL(void)
{
    assert_ldouble_equal(ceill(1.1L), 2.0L, GRANULARITY_L);
    assert_ldouble_equal(ceill(-1.1L), -1.0L, GRANULARITY_L);
    assert_ldouble_equal(ceill(0.0L), 0.0L, GRANULARITY_L);
}

void testFLOORL(void)
{
    assert_ldouble_equal(floorl(1.9L), 1.0L, GRANULARITY_L);
    assert_ldouble_equal(floorl(-1.1L), -2.0L, GRANULARITY_L);
    assert_ldouble_equal(floorl(0.0L), 0.0L, GRANULARITY_L);
}

void testFABSL(void)
{
    assert_ldouble_equal(fabsl(1.0L), 1.0L, GRANULARITY_L);
    assert_ldouble_equal(fabsl(-1.0L), 1.0L, GRANULARITY_L);
    assert_ldouble_equal(fabsl(0.0L), 0.0L, GRANULARITY_L);
}

void testPOWL(void)
{
    assert_ldouble_equal(powl(2.0L, 3.0L), 8.0L, GRANULARITY_L);
    assert_ldouble_equal(powl(4.0L, 0.5L), 2.0L, GRANULARITY_L);
    assert_ldouble_equal(powl(2.0L, -1.0L), 0.5L, GRANULARITY_L);
}

void testFMODL(void)
{
    assert_ldouble_equal(fmodl(5.3L, 2.0L), 1.3L, GRANULARITY_L);
    assert_ldouble_equal(fmodl(-5.3L, 2.0L), -1.3L, GRANULARITY_L);
}

void testLDEXPL(void)
{
    assert_ldouble_equal(ldexpl(1.0L, 3), 8.0L, GRANULARITY_L);
    assert_ldouble_equal(ldexpl(2.0L, -1), 1.0L, GRANULARITY_L);
}

void testFREXPL(void)
{
    int exp;
    long double val = frexpl(8.0L, &exp);
    assert_ldouble_equal(val, 0.5L, GRANULARITY_L);
    CU_ASSERT(exp == 4);
}

void testASINHL(void) {
    long double x = 1.5L;
    long double res = asinhl(x);
    long double expected = logl(x + sqrtl(x*x + 1.0L));
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

void testACOSHL(void) {
    long double x = 2.0L;
    long double res = acoshl(x);
    long double expected = logl(x + sqrtl(x*x - 1.0L));
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

void testATANHL(void) {
    long double x = 0.5L;
    long double res = atanhl(x);
    long double expected = 0.5L * logl((1.0L + x) / (1.0L - x));
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

void testEXP2L(void) {
    long double x = 3.0L;
    long double res = exp2l(x);
    long double expected = powl(2.0L, x);
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

void testEXPM1L(void) {
    long double x = 0.1L;
    long double res = expm1l(x);
    long double expected = expl(x) - 1.0L;
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

void testLOG1PL(void) {
    long double x = 0.1L;
    long double res = log1pl(x);
    long double expected = logl(1.0L + x);
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

void testLOG2L(void) {
    long double x = 8.0L;
    long double res = log2l(x);
    long double expected = logl(x) / logl(2.0L);
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

void testLOGBL(void) {
    long double x = 16.0L;
    long double res = logbl(x);
    int exp;
    long double mantissa = frexpl(x, &exp);
    CU_ASSERT_EQUAL(res, (long double)(exp - 1));
}

void testMODFL(void) {
    long double x = 3.14159L;
    long double intpart;
    long double fracpart = modfl(x, &intpart);
    CU_ASSERT_DOUBLE_EQUAL(intpart, 3.0L, epsilon);
    CU_ASSERT_DOUBLE_EQUAL(fracpart, x - intpart, epsilon);
}

void testCBRTL(void) {
    long double x = 27.0L;
    long double res = cbrtl(x);
    long double expected = 3.0L;
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

void testERFL(void) {
    long double x = 1.0L;
    long double res = erfl(x);
    // Just verify the value is roughly the same as erf(double)
    CU_ASSERT_DOUBLE_EQUAL((double)res, erf((double)x), 1e-7);
}

void testERFCL(void) {
    long double x = 1.0L;
    long double res = erfcl(x);
    CU_ASSERT_DOUBLE_EQUAL((double)res, erfc((double)x), 1e-7);
}

void testLGAMMAL(void) {
    long double x = 5.0L;
    long double res = lgammal(x);
    CU_ASSERT(res > 0.0L);  // Simple sanity check (lgamma(5) > 0)
}

void testTGAMMAL(void) {
    long double x = 5.0L;
    long double res = tgammal(x);
    CU_ASSERT(res > 0.0L);  // Simple sanity check (tgamma(5) > 0)
}

void testNEARBYINTL(void) {
    long double x = 2.7L;
    long double res = nearbyintl(x);
    CU_ASSERT_DOUBLE_EQUAL(res, 3.0L, epsilon);
}

void testRINTL(void) {
    long double x = 2.3L;
    long double res = rintl(x);
    CU_ASSERT_DOUBLE_EQUAL(res, 2.0L, epsilon);
}

void testLRINTL(void) {
    long double x = 3.6L;
    long int res = lrintl(x);
    CU_ASSERT_EQUAL(res, 4);
}

void testLLRINTL(void) {
    long double x = 3.6L;
    long long int res = llrintl(x);
    CU_ASSERT_EQUAL(res, 4);
}

void testROUNDL(void) {
    long double x = 3.3L;
    long double res = roundl(x);
    CU_ASSERT_DOUBLE_EQUAL(res, 3.0L, epsilon);
}

void testLROUNDL(void) {
    long double x = 4.7L;
    long res = lroundl(x);
    CU_ASSERT_EQUAL(res, 5);
}

void testLLROUNDL(void) {
    long double x = 4.7L;
    long long res = llroundl(x);
    CU_ASSERT_EQUAL(res, 5);
}

void testTRUNCL(void) {
    long double x = 3.9L;
    long double res = truncl(x);
    CU_ASSERT_DOUBLE_EQUAL(res, 3.0L, epsilon);
}

void testREMAINDERL(void) {
    long double x = 5.3L, y = 2.0L;
    long double res = remainderl(x, y);
    long double expected = x - y * roundl(x / y);
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

void testREMQUOL(void) {
    long double x = 5.3L, y = 2.0L;
    int quo;
    long double res = remquol(x, y, &quo);
    long double expected = remainderl(x, y);
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

void testCOPYSIGNL(void) {
    long double x = 3.0L, y = -1.0L;
    long double res = copysignl(x, y);
    CU_ASSERT_DOUBLE_EQUAL(res, -3.0L, epsilon);
}

void testNANL(void) {
    long double res = nanl("1");
    CU_ASSERT_TRUE(isnanl(res));
}

void testFDIML(void) {
    long double x = 5.0L, y = 3.0L;
    long double res = fdiml(x, y);
    CU_ASSERT_DOUBLE_EQUAL(res, 2.0L, epsilon);
}

void testFMAXL(void) {
    long double x = 3.0L, y = 4.0L;
    long double res = fmaxl(x, y);
    CU_ASSERT_DOUBLE_EQUAL(res, 4.0L, epsilon);
}

void testFMINL(void) {
    long double x = 3.0L, y = 4.0L;
    long double res = fminl(x, y);
    CU_ASSERT_DOUBLE_EQUAL(res, 3.0L, epsilon);
}

void testFMAL(void) {
    long double x = 2.0L, y = 3.0L, z = 4.0L;
    long double res = fmal(x, y, z);
    long double expected = x * y + z;
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

void testHYPOTL(void) {
    long double x = 3.0L;
    long double y = 4.0L;
    long double res = hypotl(x, y);
    long double expected = 5.0L; // Since sqrt(3^2 + 4^2) = 5
    CU_ASSERT_DOUBLE_EQUAL(res, expected, epsilon);
}

int main()
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("CrtMath_Suite", init_suite1, clean_suite1);
    if (NULL == pSuite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "test of sinl()", testSINL)) ||
        (NULL == CU_add_test(pSuite, "test of cosl()", testCOSL)) ||
        (NULL == CU_add_test(pSuite, "test of tanl()", testTANL)) ||
        (NULL == CU_add_test(pSuite, "test of asinl()", testASINL)) ||
        (NULL == CU_add_test(pSuite, "test of acosl()", testACOSL)) ||
        (NULL == CU_add_test(pSuite, "test of atanl()", testATANL)) ||
        (NULL == CU_add_test(pSuite, "test of atan2l()", testATAN2L)) ||
        (NULL == CU_add_test(pSuite, "test of sinhl()", testSINHL)) ||
        (NULL == CU_add_test(pSuite, "test of coshl()", testCOSHL)) ||
        (NULL == CU_add_test(pSuite, "test of tanhl()", testTANHL)) ||
        (NULL == CU_add_test(pSuite, "test of asinhl()", testASINHL)) ||
        (NULL == CU_add_test(pSuite, "test of acoshl()", testACOSHL)) ||
        (NULL == CU_add_test(pSuite, "test of atanhl()", testATANHL)) ||
        (NULL == CU_add_test(pSuite, "test of sqrtl()", testSQRTL)) ||
        (NULL == CU_add_test(pSuite, "test of powl()", testPOWL)) ||
        (NULL == CU_add_test(pSuite, "test of hypotl()", testHYPOTL)) ||
        (NULL == CU_add_test(pSuite, "test of expl()", testEXPL)) ||
        (NULL == CU_add_test(pSuite, "test of exp2l()", testEXP2L)) ||
        (NULL == CU_add_test(pSuite, "test of expm1l()", testEXPM1L)) ||
        (NULL == CU_add_test(pSuite, "test of frexpl()", testFREXPL)) ||
        (NULL == CU_add_test(pSuite, "test of ldexpl()", testLDEXPL)) ||
        (NULL == CU_add_test(pSuite, "test of logl()", testLOGL)) ||
        (NULL == CU_add_test(pSuite, "test of log10l()", testLOG10L)) ||
        (NULL == CU_add_test(pSuite, "test of log1pl()", testLOG1PL)) ||
        (NULL == CU_add_test(pSuite, "test of log2l()", testLOG2L)) ||
        (NULL == CU_add_test(pSuite, "test of logbl()", testLOGBL)) ||
        (NULL == CU_add_test(pSuite, "test of modfl()", testMODFL)) ||
        (NULL == CU_add_test(pSuite, "test of cbrtl()", testCBRTL)) ||
        (NULL == CU_add_test(pSuite, "test of fabsl()", testFABSL)) ||
        (NULL == CU_add_test(pSuite, "test of erfl()", testERFL)) ||
        (NULL == CU_add_test(pSuite, "test of erfcl()", testERFCL)) ||
        (NULL == CU_add_test(pSuite, "test of lgammal()", testLGAMMAL)) ||
        (NULL == CU_add_test(pSuite, "test of tgammal()", testTGAMMAL)) ||
        (NULL == CU_add_test(pSuite, "test of ceill()", testCEILL)) ||
        (NULL == CU_add_test(pSuite, "test of floorl()", testFLOORL)) ||
        (NULL == CU_add_test(pSuite, "test of nearbyintl()", testNEARBYINTL)) ||
        (NULL == CU_add_test(pSuite, "test of rintl()", testRINTL)) ||
        (NULL == CU_add_test(pSuite, "test of lrintl()", testLRINTL)) ||
        (NULL == CU_add_test(pSuite, "test of llrintl()", testLLRINTL)) ||
        (NULL == CU_add_test(pSuite, "test of roundl()", testROUNDL)) ||
        (NULL == CU_add_test(pSuite, "test of lroundl()", testLROUNDL)) ||
        (NULL == CU_add_test(pSuite, "test of llroundl()", testLLROUNDL)) ||
        (NULL == CU_add_test(pSuite, "test of truncl()", testTRUNCL)) ||
        (NULL == CU_add_test(pSuite, "test of fmodl()", testFMODL)) ||
        (NULL == CU_add_test(pSuite, "test of remainderl()", testREMAINDERL)) ||
        (NULL == CU_add_test(pSuite, "test of remquol()", testREMQUOL)) ||
        (NULL == CU_add_test(pSuite, "test of copysignl()", testCOPYSIGNL)) ||
        (NULL == CU_add_test(pSuite, "test of nanl()", testNANL)) ||
        (NULL == CU_add_test(pSuite, "test of fdiml()", testFDIML)) ||
        (NULL == CU_add_test(pSuite, "test of fmaxl()", testFMAXL)) ||
        (NULL == CU_add_test(pSuite, "test of fminl()", testFMINL)) ||
        (NULL == CU_add_test(pSuite, "test of fmal()", testFMAL))) 
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTMathUnitTests");
    CU_set_output_filename("CRT-Math");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    
    return CU_get_error();
}

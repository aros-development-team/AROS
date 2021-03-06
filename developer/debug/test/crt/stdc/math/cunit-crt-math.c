/*
    Copyright (C) 2021, The AROS Development Team. All rights reserved.
*/

#include <math.h>
#include <fenv.h>
#include <errno.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#define GRANULARITY 0.000001

/*
 * C99 double versions of functions
 */

#if 0
todo:
int     ilogb(double) __pure2;
double  scalbn(double, int);
double  scalbln(double, long);
double  nextafter(double, double);
double  nexttoward(double, long double);
#endif

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

void testSIN(void)
{
    CU_ASSERT_DOUBLE_EQUAL(sin(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(sin(5), -0.958924, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(sin(-5), 0.958924, GRANULARITY);
}

void testCOS(void)
{
    CU_ASSERT_DOUBLE_EQUAL(cos(0), 1.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(cos(5), 0.283662, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(cos(-5), 0.283662, GRANULARITY);
}

void testTAN(void)
{
    CU_ASSERT_DOUBLE_EQUAL(tan(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(tan(5), -3.380515, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(tan(-5), 3.380515, GRANULARITY);
    
    /* tan(pi/2) is mathemathically not defined
     * but we get a large value */
    CU_ASSERT(tan(M_PI_2) > 15000000000000000.0);
}

void testASIN(void)
{
    CU_ASSERT_DOUBLE_EQUAL(asin(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(asin(-0.5), -0.523599, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(asin(1), M_PI_2, GRANULARITY);
    
    errno = 0;
    CU_ASSERT(isnan(asin(1.1)));
    CU_ASSERT(errno == EDOM);
}

void testACOS(void)
{
    CU_ASSERT_DOUBLE_EQUAL(acos(0), M_PI_2, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(acos(-0.5), 2.094395, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(acos(1), 0, GRANULARITY);
    
    errno = 0;
    CU_ASSERT(isnan(acos(1.1)));
    CU_ASSERT(errno == EDOM);
}

void testATAN(void)
{
    CU_ASSERT_DOUBLE_EQUAL(atan(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan(-0.5), -0.463648, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan(1), 0.785398, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan(5), 1.373401, GRANULARITY);
}

void testATAN2(void)
{
                              /* y, x */
    CU_ASSERT_DOUBLE_EQUAL(atan2(0, 2), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(2, 2), 0.785398, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(2, 0), 1.570796, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(2, -2), 2.356194, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(0, -2), 3.141593, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(-2, -2), -2.356194, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(-2, 0), -1.570796, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(-2, 2), -0.785398, GRANULARITY);
    
    CU_ASSERT_DOUBLE_EQUAL(atan2(0, 0), 0.0, GRANULARITY);
}

void testSINH(void)
{
    CU_ASSERT_DOUBLE_EQUAL(sinh(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(sinh(5), 74.203211, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(sinh(-5), -74.203211, GRANULARITY);
}

void testCOSH(void)
{
    CU_ASSERT_DOUBLE_EQUAL(cosh(0), 1.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(cosh(5), 74.209949, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(cosh(-5), 74.209949, GRANULARITY);
}

void testTANH(void)
{
    CU_ASSERT_DOUBLE_EQUAL(tanh(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(tanh(5), 0.999909, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(tanh(-5), -0.999909, GRANULARITY);
}

void testASINH(void)
{
    CU_ASSERT_DOUBLE_EQUAL(asinh(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(asinh(5), 2.312438, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(asinh(-5), -2.312438, GRANULARITY);
}

void testACOSH(void)
{
    double x;
    
    x = 0;
    errno = 0;
    CU_ASSERT(isnan(acosh(x)));
    CU_ASSERT(errno == EDOM);

    CU_ASSERT_DOUBLE_EQUAL(acosh(5), 2.292432, GRANULARITY);
    
    x = -5;
    errno = 0;
    CU_ASSERT(isnan(acosh(x)));
    CU_ASSERT(errno == EDOM);
}

void testATANH(void)
{
    double x;
    
    CU_ASSERT_DOUBLE_EQUAL(atanh(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atanh(0.5), 0.549306, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atanh(-0.5), -0.549306, GRANULARITY);

    x = 5;
    errno = 0;
    CU_ASSERT(isnan(atanh(x)));
    CU_ASSERT(errno == EDOM);

    x = -5;
    errno = 0;
    CU_ASSERT(isnan(atanh(x)));
    CU_ASSERT(errno == EDOM);
}

void testSQRT(void)
{
    CU_ASSERT_DOUBLE_EQUAL(sqrt(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(sqrt(5), 2.236068, GRANULARITY);

    errno = 0;
    CU_ASSERT(isnan(sqrt(-1)));
    CU_ASSERT(errno == EDOM);
}

void testHYPOT(void)
{
    CU_ASSERT_DOUBLE_EQUAL(hypot(2.1, 3.1), 3.744329, GRANULARITY);
}

void testPOW(void)
{
    CU_ASSERT_DOUBLE_EQUAL(pow(2.1, 3.1), 9.974239, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(pow(2.1, -3.1), 0.100258, GRANULARITY);
    CU_ASSERT(isnan(pow(-2.1, 3.1)));
    CU_ASSERT(isnan(pow(-2.1, -3.1)));
    CU_ASSERT_DOUBLE_EQUAL(pow(0, 3.1), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(pow(2.1, 0), 1.0, GRANULARITY);
}

void testEXP(void)
{
    CU_ASSERT_DOUBLE_EQUAL(exp(0), 1.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(exp(5), 148.413159, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(exp(-5), 0.006738, GRANULARITY);
}

void testEXP2(void)
{
    CU_ASSERT_DOUBLE_EQUAL(exp2(0), 1.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(exp2(5), 32.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(exp2(-5), 0.031250, GRANULARITY);
}

void testEXPM1(void)
{
    CU_ASSERT_DOUBLE_EQUAL(expm1(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(expm1(5), 147.413159, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(expm1(-5), -0.993262, GRANULARITY);
}

void testFREXP(void)
{
    int exp;
    CU_ASSERT_DOUBLE_EQUAL(frexp(1024, &exp), 0.5, GRANULARITY);
    CU_ASSERT(11 == exp);
    CU_ASSERT_DOUBLE_EQUAL(frexp(-1024, &exp), -0.5, GRANULARITY);
    CU_ASSERT(11 == exp);
    CU_ASSERT_DOUBLE_EQUAL(frexp(0, &exp), 0.0, GRANULARITY);
    CU_ASSERT(0 == exp);
}

void testLDEXP(void)
{
    CU_ASSERT_DOUBLE_EQUAL(ldexp(0, 0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(ldexp(5, 4), 80.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(ldexp(-5, 4), -80.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(ldexp(5, -4), 0.312500, GRANULARITY);
}

void testLOG(void)
{
    CU_ASSERT_DOUBLE_EQUAL(log(4), 1.386294, GRANULARITY);

    errno = 0;
    CU_ASSERT(isnan(log(-3)));
    CU_ASSERT(errno == EDOM);
}

void testLOG10(void)
{
    CU_ASSERT_DOUBLE_EQUAL(log10(4), 0.602060, GRANULARITY);

    errno = 0;
    CU_ASSERT(isnan(log10(-3)));
    CU_ASSERT(errno == EDOM);
}

void testLOG1P(void)
{
    CU_ASSERT_DOUBLE_EQUAL(log1p(4), 1.609438, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(log1p(0), 0.0, GRANULARITY);

    errno = 0;
    CU_ASSERT(isnan(log1p(-3)));
    CU_ASSERT(errno == EDOM);
}

void testLOG2(void)
{
    CU_ASSERT_DOUBLE_EQUAL(log2(4), 2.0, GRANULARITY);

    errno = 0;
    CU_ASSERT(isnan(log2(-3)));
    CU_ASSERT(errno == EDOM);
}

void testLOGB(void)
{
    CU_ASSERT_DOUBLE_EQUAL(logb(5), 2.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(logb(-5), 2.0, GRANULARITY);
}

void testMODF(void)
{
    double i;
    CU_ASSERT_DOUBLE_EQUAL(modf(5.5, &i), 0.5, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(i, 5, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(modf(-5.5, &i), -0.5, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(i, -5.0, GRANULARITY);
}

void testCBRT(void)
{
    CU_ASSERT_DOUBLE_EQUAL(cbrt(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(cbrt(5), 1.709976, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(cbrt(-5), -1.709976, GRANULARITY);
}

void testFABS(void)
{
    CU_ASSERT_DOUBLE_EQUAL(fabs(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fabs(5), 5.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fabs(-5), 5.0, GRANULARITY);
}

void testERF(void)
{
    CU_ASSERT_DOUBLE_EQUAL(erf(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(erf(1), 0.842701, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(erf(-1), -0.842701, GRANULARITY);
}

void testERFC(void)
{
    CU_ASSERT_DOUBLE_EQUAL(erfc(0), 1.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(erfc(1), 0.157299, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(erfc(-1), 1.842701, GRANULARITY);
}

void testLGAMMA(void)
{
    CU_ASSERT_DOUBLE_EQUAL(lgamma(1), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(lgamma(2), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(lgamma(5), 3.178054, GRANULARITY);
}

void testTGAMMA(void)
{
    CU_ASSERT_DOUBLE_EQUAL(tgamma(1), 1.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(tgamma(2), 1.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(tgamma(5), 24.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(tgamma(5.3), 38.077976, GRANULARITY);
}

void testCEIL(void)
{
    CU_ASSERT_DOUBLE_EQUAL(ceil(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(ceil(5.3), 6.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(ceil(-7.6), -7.0, GRANULARITY);
}

void testFLOOR(void)
{
    CU_ASSERT_DOUBLE_EQUAL(floor(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(floor(5.3), 5.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(floor(-7.6), -8.0, GRANULARITY);
}

void testNEARBYINT(void)
{
    fesetround(FE_TONEAREST);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(5.3), 5.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(-7.6), -8.0, GRANULARITY);

    fesetround(FE_DOWNWARD);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(5.3), 5.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(-7.6), -8.0, GRANULARITY);

    fesetround(FE_UPWARD);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(5.3), 6.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(-7.6), -7.0, GRANULARITY);

    fesetround(FE_TOWARDZERO);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(5.3), 5.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(nearbyint(-7.6), -7.0, GRANULARITY);
}

void testRINT(void)
{
    fesetround(FE_TONEAREST);
    CU_ASSERT_DOUBLE_EQUAL(rint(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(rint(5.3), 5.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(rint(-7.6), -8.0, GRANULARITY);

    fesetround(FE_DOWNWARD);
    CU_ASSERT_DOUBLE_EQUAL(rint(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(rint(5.3), 5.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(rint(-7.6), -8.0, GRANULARITY);

    fesetround(FE_UPWARD);
    CU_ASSERT_DOUBLE_EQUAL(rint(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(rint(5.3), 6.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(rint(-7.6), -7.0, GRANULARITY);

    fesetround(FE_TOWARDZERO);
    CU_ASSERT_DOUBLE_EQUAL(rint(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(rint(5.3), 5.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(rint(-7.6), -7.0, GRANULARITY);
}

void testLRINT(void)
{
    fesetround(FE_TONEAREST);
    CU_ASSERT(0 == lrint(0));
    CU_ASSERT(5 == lrint(5.3));
    CU_ASSERT(-8 == lrint(-7.6));

    fesetround(FE_DOWNWARD);
    CU_ASSERT(0 == lrint(0));
    CU_ASSERT(5 == lrint(5.3));
    CU_ASSERT(-8 == lrint(-7.6));

    fesetround(FE_UPWARD);
    CU_ASSERT(0 == lrint(0));
    CU_ASSERT(6 == lrint(5.3));
    CU_ASSERT(-7 == lrint(-7.6));

    fesetround(FE_TOWARDZERO);
    CU_ASSERT(0 == lrint(0));
    CU_ASSERT(5 == lrint(5.3));
    CU_ASSERT(-7 == lrint(-7.6));
}

void testLLRINT(void)
{
    fesetround(FE_TONEAREST);
    CU_ASSERT(0 == llrint(0));
    CU_ASSERT(5 == llrint(5.3));
    CU_ASSERT(-8 == llrint(-7.6));

    fesetround(FE_DOWNWARD);
    CU_ASSERT(0 == llrint(0));
    CU_ASSERT(5 == llrint(5.3));
    CU_ASSERT(-8 == llrint(-7.6));

    fesetround(FE_UPWARD);
    CU_ASSERT(0 == llrint(0));
    CU_ASSERT(6 == llrint(5.3));
    CU_ASSERT(-7 == llrint(-7.6));

    fesetround(FE_TOWARDZERO);
    CU_ASSERT(0 == llrint(0));
    CU_ASSERT(5 == llrint(5.3));
    CU_ASSERT(-7 == llrint(-7.6));
}

void testROUND(void)
{
    CU_ASSERT_DOUBLE_EQUAL(round(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(round(5.3), 5.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(round(-7.6), -8.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(round(5.5), 6.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(round(-7.5), -8.0, GRANULARITY);
}

void testLROUND(void)
{
    CU_ASSERT(0 == lround(0));
    CU_ASSERT(5 == lround(5.3));
    CU_ASSERT(-8 == lround(-7.6));
    CU_ASSERT(6 == lround(5.5));
    CU_ASSERT(-8 == lround(-7.5));
}

void testLLROUND(void)
{
    CU_ASSERT(0 == llround(0));
    CU_ASSERT(5 == llround(5.3));
    CU_ASSERT(-8 == llround(-7.6));
    CU_ASSERT(6 == llround(5.5));
    CU_ASSERT(-8 == llround(-7.5));
}

void testTRUNC(void)
{
    CU_ASSERT_DOUBLE_EQUAL(trunc(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(trunc(5.3), 5.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(trunc(-7.6), -7.0, GRANULARITY);
}

void testFMOD(void)
{
    CU_ASSERT_DOUBLE_EQUAL(fmod(2.1, 5.3), 2.1, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fmod(5.3, 2.1), 1.1, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fmod(-20.4, 2.1), -1.5, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fmod(20.4, -2.1), 1.5, GRANULARITY);
}

void testREMAINDER(void)
{
    CU_ASSERT_DOUBLE_EQUAL(remainder(2.1, 5.3), 2.1, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(remainder(5.3, 2.1), -1, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(remainder(-20.4, 2.1), 0.6, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(remainder(20.4, -2.1), -0.6, GRANULARITY);
}

void testREMQUO(void)
{
    int quo;
    CU_ASSERT_DOUBLE_EQUAL(remquo(2.1, 5.3, &quo), 2.1, GRANULARITY);
    CU_ASSERT(quo >= 0);
    CU_ASSERT_DOUBLE_EQUAL(remquo(5.3, 2.1, &quo), -1, GRANULARITY);
    CU_ASSERT(quo >= 0);
    CU_ASSERT_DOUBLE_EQUAL(remquo(-20.4, 2.1, &quo), 0.6, GRANULARITY);
    CU_ASSERT(quo < 0);
    CU_ASSERT_DOUBLE_EQUAL(remquo(20.4, -2.1, &quo), -0.6, GRANULARITY);
    CU_ASSERT(quo < 0);
}

void testCOPYSIGN(void)
{
    CU_ASSERT_DOUBLE_EQUAL(copysign(2.1, 5.3), 2.1, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(copysign(-5.3, -2.1), -5.3, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(copysign(-20.4, 2.1), 20.4, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(copysign(20.4, -2.1), -20.4, GRANULARITY);
}

void testNAN(void)
{
    CU_ASSERT(isnan(nan("")));
    CU_ASSERT(isnan(nan("-1")));
    CU_ASSERT(isnan(nan("2")));
}

void testFDIM(void)
{
    CU_ASSERT_DOUBLE_EQUAL(fdim(20.3, 2.1), 18.2, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fdim(-5.3, -2.1), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fdim(-20.4, 2.1), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fdim(20.4, -2.1), 22.5, GRANULARITY);
}

void testFMAX(void)
{
    CU_ASSERT_DOUBLE_EQUAL(fmax(20.3, 2.1), 20.3, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fmax(-5.3, -2.1), -2.1, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fmax(-20.4, 2.1), 2.1, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fmax(20.4, -2.1), 20.4, GRANULARITY);
}

void testFMIN(void)
{
    CU_ASSERT_DOUBLE_EQUAL(fmin(20.3, 2.1), 2.1, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fmin(-5.3, -2.1), -5.3, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fmin(-20.4, 2.1), -20.4, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fmin(20.4, -2.1), -2.1, GRANULARITY);
}

void testFMA(void)
{
    CU_ASSERT_DOUBLE_EQUAL(fma(20.3, 2.1, 4.2), 46.83, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fma(-5.3, -2.1, 4.2), 15.33, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fma(-20.4, 2.1, 4.2), -38.64, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(fma(20.4, -2.1, -4.2), -47.04, GRANULARITY);
}

int main()
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("Suite_1", init_suite1, clean_suite1);
    if (NULL == pSuite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "test of sin()", testSIN)) ||
        (NULL == CU_add_test(pSuite, "test of cos()", testCOS)) ||
        (NULL == CU_add_test(pSuite, "test of tan()", testTAN)) ||
        (NULL == CU_add_test(pSuite, "test of asin()", testASIN)) ||
        (NULL == CU_add_test(pSuite, "test of acos()", testACOS)) ||
        (NULL == CU_add_test(pSuite, "test of atan()", testATAN)) ||
        (NULL == CU_add_test(pSuite, "test of atan2()", testATAN2)) ||
        (NULL == CU_add_test(pSuite, "test of sinh()", testSINH)) ||
        (NULL == CU_add_test(pSuite, "test of cosh()", testCOSH)) ||
        (NULL == CU_add_test(pSuite, "test of tanh()", testTANH)) ||
        (NULL == CU_add_test(pSuite, "test of asinh()", testASINH)) ||
        (NULL == CU_add_test(pSuite, "test of acosh()", testACOSH)) ||
        (NULL == CU_add_test(pSuite, "test of atanh()", testATANH)) ||
        (NULL == CU_add_test(pSuite, "test of sqrt()", testSQRT)) ||
        (NULL == CU_add_test(pSuite, "test of pow()", testPOW)) ||
        (NULL == CU_add_test(pSuite, "test of hypot()", testHYPOT)) ||
        (NULL == CU_add_test(pSuite, "test of exp()", testEXP)) ||
        (NULL == CU_add_test(pSuite, "test of exp2()", testEXP2)) ||
        (NULL == CU_add_test(pSuite, "test of expm1()", testEXPM1)) ||
        (NULL == CU_add_test(pSuite, "test of frexp()", testFREXP)) ||
        (NULL == CU_add_test(pSuite, "test of ldexp()", testLDEXP)) ||
        (NULL == CU_add_test(pSuite, "test of log()", testLOG)) ||
        (NULL == CU_add_test(pSuite, "test of log()", testLOG10)) ||
        (NULL == CU_add_test(pSuite, "test of log1p()", testLOG1P)) ||
        (NULL == CU_add_test(pSuite, "test of log2()", testLOG2)) ||
        (NULL == CU_add_test(pSuite, "test of logb()", testLOGB)) ||
        (NULL == CU_add_test(pSuite, "test of modf()", testMODF)) ||
        (NULL == CU_add_test(pSuite, "test of modf()", testCBRT)) ||
        (NULL == CU_add_test(pSuite, "test of fabs()", testFABS)) ||
        (NULL == CU_add_test(pSuite, "test of erf()", testERF)) ||
        (NULL == CU_add_test(pSuite, "test of erfc()", testERFC)) ||
        (NULL == CU_add_test(pSuite, "test of lgamma()", testLGAMMA)) ||
        (NULL == CU_add_test(pSuite, "test of tgamma()", testTGAMMA)) ||
        (NULL == CU_add_test(pSuite, "test of ceil()", testCEIL)) ||
        (NULL == CU_add_test(pSuite, "test of floor()", testFLOOR)) ||
        (NULL == CU_add_test(pSuite, "test of nearbyint()", testNEARBYINT)) ||
        (NULL == CU_add_test(pSuite, "test of rint()", testRINT)) ||
        (NULL == CU_add_test(pSuite, "test of lrint()", testLRINT)) ||
        (NULL == CU_add_test(pSuite, "test of llrint()", testLLRINT)) ||
        (NULL == CU_add_test(pSuite, "test of round()", testROUND)) ||
        (NULL == CU_add_test(pSuite, "test of lround()", testLROUND)) ||
        (NULL == CU_add_test(pSuite, "test of llround()", testLLROUND)) ||
        (NULL == CU_add_test(pSuite, "test of trunc()", testTRUNC)) ||
        (NULL == CU_add_test(pSuite, "test of fmod()", testFMOD)) ||
        (NULL == CU_add_test(pSuite, "test of remainder()", testREMAINDER)) ||
        (NULL == CU_add_test(pSuite, "test of remquo()", testREMQUO)) ||
        (NULL == CU_add_test(pSuite, "test of copysign()", testCOPYSIGN)) ||
        (NULL == CU_add_test(pSuite, "test of nan()", testNAN)) ||
        (NULL == CU_add_test(pSuite, "test of fdim()", testFDIM)) ||
        (NULL == CU_add_test(pSuite, "test of fmax()", testFMAX)) ||
        (NULL == CU_add_test(pSuite, "test of fmin()", testFMIN)) ||
        (NULL == CU_add_test(pSuite, "test of fma()", testFMA)))
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

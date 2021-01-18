/*
    Copyright © 2021, The AROS Development Team. All rights reserved.
*/

#include <math.h>
#include <errno.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#define GRANULARITY 0.000001

#if 0
/*
 * C99 double versions of functions
 */

done:
double	sin(double);
double	cos(double);
double	tan(double);
double	acos(double);
double	asin(double);
double	atan(double);
double	atan2(double, double);
double	sqrt(double);

double	sinh(double);
double	cosh(double);
double	tanh(double);

double	asinh(double);
double	acosh(double);
double	atanh(double);
double	exp(double);
double	exp2(double);

todo:
double	expm1(double);
double	frexp(double, int *);	/* fundamentally !__pure2 */
int	ilogb(double) __pure2;
double	ldexp(double, int);
double	log(double);
double	log10(double);
double	log1p(double);
double	log2(double);
double	logb(double);
double	modf(double, double *);	/* fundamentally !__pure2 */
double	scalbn(double, int);
double	scalbln(double, long);

double	cbrt(double);
double	fabs(double) __pure2;
double	hypot(double, double);
double	pow(double, double);

double	erf(double);
double	erfc(double);
double	lgamma(double);
double	tgamma(double);

double	ceil(double);
double	floor(double);
double	nearbyint(double);
double	rint(double);
long	lrint(double);
long long llrint(double);
double	round(double);
long	lround(double);
long long llround(double);
double	trunc(double);

double	fmod(double, double);
double	remainder(double, double);
double	remquo(double, double, int *);

double	copysign(double, double) __pure2;
double  nan(const char *tagp);
double	nextafter(double, double);
double	nexttoward(double, long double);

double	fdim(double, double);
double	fmax(double, double) __pure2;
double	fmin(double, double) __pure2;

double	fma(double, double, double);

/* aliases */
double	gamma(double);
double	drem(double, double);
int	finite(double) __pure2;
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
    double x; // silence codacity
    
    CU_ASSERT_DOUBLE_EQUAL(asin(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(asin(-0.5), -0.523599, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(asin(1), M_PI_2, GRANULARITY);
    
    x = 1.1;
    errno = 0;
    CU_ASSERT(isnan(asin(x)));
    CU_ASSERT(errno == EDOM);
}

void testACOS(void)
{
    double x;
    
    CU_ASSERT_DOUBLE_EQUAL(acos(0), M_PI_2, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(acos(-0.5), 2.094395, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(acos(1), 0, GRANULARITY);
    
    x = 1.1;
    errno = 0;
    CU_ASSERT(isnan(acos(x)));
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
    double x;
                              /* y, x */
    CU_ASSERT_DOUBLE_EQUAL(atan2(0, 2), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(2, 2), 0.785398, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(2, 0), 1.570796, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(2, -2), 2.356194, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(0, -2), 3.141593, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(-2, -2), -2.356194, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(-2, 0), -1.570796, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(atan2(-2, 2), -0.785398, GRANULARITY);
    
    x = 0;
    CU_ASSERT_DOUBLE_EQUAL(atan2(x, x), 0.0, GRANULARITY);
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
    double x;
    
    CU_ASSERT_DOUBLE_EQUAL(sqrt(0), 0.0, GRANULARITY);
    CU_ASSERT_DOUBLE_EQUAL(sqrt(5), 2.236068, GRANULARITY);

    x = -1;
    errno = 0;
    CU_ASSERT(isnan(sqrt(x)));
    CU_ASSERT(errno == EDOM);
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
        (NULL == CU_add_test(pSuite, "test of exp()", testEXP)) ||
        (NULL == CU_add_test(pSuite, "test of exp2()", testEXP2)))
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

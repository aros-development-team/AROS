#ifndef _STDC_TGMATH_H_
//#define _STDC_TGMATH_H_
/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Standard C Library: Unary functions defined for real and complex values..
*/

#include <aros/system.h>

/*
 * Trigonometric functions.
 */

/* Arc cosine/since/tangent of X.  */
//#define acos(num)
//#define asin(num)
//#define atan(num)
/* Arc tangent of Y/X.  */
//#define atan2(num1, num2)

/* Cosine/Sine/Tangent of X.  */
//#define cos(num)
//#define sin(num)
//#define tan(num)

/*
 * Hyperbolic functions.
 */

/* Hyperbolic arc cosine/sine/tangent of X.  */
//#define acosh(num)
//#define asinh(num)
//#define atanh(num)

/* Hyperbolic cosine/sine/tangent of X.  */
//#define cosh(num)
//#define sinh(num)
//#define tanh(num)

/*
 * Exponential and logarithmic functions.
 */

/* Exponential function of X.  */
//#define exp(num)

/* Break VALUE into a normalized fraction and an integral power of 2.  */
//#define frexp(num1, num2)

/* X times (two to the EXP power).  */
//#define ldexp(num1, num2)

/* Natural logarithm of X.  */
//#define log(num)

/* Base-ten logarithm of X.  */
//#define log10(num)

/* Return exp(X) - 1.  */
//#define expm1(num)

/* Return log(1 + X).  */
//#define log1p(num)

/* Return the base 2 signed integral exponent of X.  */
//#define logb(num)

/* Compute base-2 exponential of X.  */
//#define exp2(num)

/* Compute base-2 logarithm of X.  */
//#define log2(num)


/*
 * Power functions.
 */

/* Return X to the Y power.  */
//#define pow(num1, num2)

/* Return the square root of X.  */
//#define sqrt(num)

/* Return `sqrt(X*X + Y*Y)'.  */
//#define hypot(num1, num2)

/* Return the cube root of X.  */
//#define cbrt(num)

/*
 * Nearest integer, absolute value, and remainder functions.
 */

/* Smallest integral value not less than X.  */
//#define ceil(num)

/* Absolute value of X.  */
//#define fabs(num)

/* Largest integer not greater than X.  */
//#define floor(num)

/* Floating-point modulo remainder of X/Y.  */
//#define fmod(num1, num2)

/* Round X to integral valuein floating-point format using current
   rounding direction, but do not raise inexact exception.  */
//#define nearbyint(num)

/* Round X to nearest integral value, rounding halfway cases away from
   zero.  */
//#define round(num)

/* Round X to the integral value in floating-point format nearest but
   not larger in magnitude.  */
//#define trunc(num)

/* Compute remainder of X and Y and put in *QUO a value with sign of x/y
   and magnitude congruent `mod 2^n' to the magnitude of the integral
   quotient x/y, with n >= 3.  */
//#define remquo(num1, num2, num3)

/* Round X to nearest integral value according to current rounding
   direction.  */
//#define lrint(num)
//#define llrint(num)

/* Round X to nearest integral value, rounding halfway cases away from
   zero.  */
//#define lround(num)
//#define llround(num)


/* Return X with its signed changed to Y's.  */
//#define copysign(num1, num2)

/* Error and gamma functions.  */
//#define erf(num)
//#define erfc(num)
//#define tgamma(num)
//#define lgamma(num)


/* Return the integer nearest X in the direction of the
   prevailing rounding mode.  */
//#define rint(num)

/* Return X + epsilon if X < Y, X - epsilon if X > Y.  */
//#define nextafter(num1, num2)
//#define nexttoward(num1, num2)

/* Return the remainder of integer divison X / Y with infinite precision.  */
//#define remainder(num1, num2)

/* Return X times (2 to the Nth power).  */
//#define scalb(num1, num2)

/* Return X times (2 to the Nth power).  */
//#define scalbn(num1, num2)

/* Return X times (2 to the Nth power).  */
//#define scalbln(num1, num2)

/* Return the binary exponent of X, which must be nonzero.  */
//#define ilogb(num)


/* Return positive difference between X and Y.  */
//#define fdim(num1, num2)

/* Return maximum numeric value from X and Y.  */
//#define fmax(num1, num2)

/* Return minimum numeric value from X and Y.  */
//#define fmin(num1, num2)


/* Multiply-add function computed as a ternary operation.  */
//#define fma(num1, num2, num3)

/*
 * Absolute value, conjugates, and projection.
 */

/* Argument value of Z.  */
//#define carg(num)

/* Complex conjugate of Z.  */
//#define conj(num)

/* Projection of Z onto the Riemann sphere.  */
//#define cproj(num)


/*
 * Decomposing complex values.
 */

/* Imaginary part of Z.  */
//#define cimag(num)

/* Real part of Z.  */
//#define creal(num)

#endif /* _STDC_TGMATH_H_ */

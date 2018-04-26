#ifndef _STDC_COMPLEX_H_
#define _STDC_COMPLEX_H_
/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Standard C Library: C99 complex.h
*/

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include <aros/system.h>

#ifdef __GNUC__
#ifndef __GNUC_PREREQ__
#define __GNUC_PREREQ__ __GNUC_PREREQ
#endif

#if __STDC_VERSION__ < 199901
#define	_Complex	__complex__
#endif
#define	_Complex_I	((float _Complex)1.0i)
#endif

#define	complex		_Complex
#define	I		_Complex_I

/*
 * Macros that can be used to construct complex values.
 *
 * The C99 standard intends x+I*y to be used for this, but x+I*y is
 * currently unusable in general since gcc introduces many overflow,
 * underflow, sign and efficiency bugs by rewriting I*y as
 * (0.0+I)*(y+0.0*I) and laboriously computing the full complex product.
 * In particular, I*Inf is corrupted to NaN+I*Inf, and I*-0 is corrupted
 * to -0.0+I*0.0.
 *
 * In C11, a CMPLX(x,y) macro was added to circumvent this limitation,
 * and gcc 4.7 added a __builtin_complex feature to simplify implementation
 * of CMPLX in libc, so we can take advantage of these features if they
 * are available. Clang simply allows complex values to be constructed
 * using a compound literal.
 *
 * If __builtin_complex is not available, resort to using inline
 * functions instead. These can unfortunately not be used to construct
 * compile-time constants.
 *
 * C99 specifies that complex numbers have the same representation as
 * an array of two elements, where the first element is the real part
 * and the second element is the imaginary part.
 */

#ifdef __clang__
#  define CMPLXF(x, y) ((float complex){x, y})
#  define CMPLX(x, y) ((double complex){x, y})
#  define CMPLXL(x, y) ((long double complex){x, y})
#elif __GNUC_PREREQ__(4, 7)
#  define CMPLXF(x,y) __builtin_complex ((float) (x), (float) (y))
#  define CMPLX(x,y) __builtin_complex ((double) (x), (double) (y))
#  define CMPLXL(x,y) __builtin_complex ((long double) (x), (long double) (y))
#endif

__BEGIN_DECLS

/*
 * Double versions of C99 functions
 */
double complex cacos(double complex);
double complex casin(double complex);
double complex catan(double complex);
double complex ccos(double complex);
double complex csin(double complex);
double complex ctan(double complex);
double complex cacosh(double complex);
double complex casinh(double complex);
double complex catanh(double complex);
double complex ccosh(double complex);
double complex csinh(double complex);
double complex ctanh(double complex);
double complex cexp(double complex);
double complex clog(double complex);
double cabs(double complex);
double complex cpow(double complex, double complex);
double complex csqrt(double complex);
double carg(double complex);
double cimag(double complex);
double complex conj(double complex);
double complex cproj(double complex);
double creal(double complex);

/*
 * Float versions of C99 functions
 */
float complex cacosf(float complex);
float complex casinf(float complex);
float complex catanf(float complex);
float complex ccosf(float complex);
float complex csinf(float complex);
float complex ctanf(float complex);
float complex cacoshf(float complex);
float complex casinhf(float complex);
float complex catanhf(float complex);
float complex ccoshf(float complex);
float complex csinhf(float complex);
float complex ctanhf(float complex);
float complex cexpf(float complex);
float complex clogf(float complex);
float cabsf(float complex);
float complex cpowf(float complex, float complex);
float complex csqrtf(float complex);
float cargf(float complex);
float cimagf(float complex);
float complex conjf(float complex);
float complex cprojf(float complex);
float crealf(float complex);

/*
 * Long double versions of C99 functions
 */
long double complex cacosl(long double complex);
long double complex casinl(long double complex);
long double complex catanl(long double complex);
long double complex ccosl(long double complex);
long double complex csinl(long double complex);
long double complex ctanl(long double complex);
long double complex cacoshl(long double complex);
long double complex casinhl(long double complex);
long double complex catanhl(long double complex);
long double complex ccoshl(long double complex);
long double complex csinhl(long double complex);
long double complex ctanhl(long double complex);
long double complex cexpl(long double complex);
long double complex clogl(long double complex);
long double cabsl(long double complex);
long double complex cpowl(long double complex, long double complex);
long double complex csqrtl(long double complex);
long double cargl(long double complex);
long double cimagl(long double complex);
long double complex conjl(long double complex);
long double complex cprojl(long double complex);
long double creall(long double complex);

__END_DECLS

#endif /* _STDC_COMPLEX_H_ */

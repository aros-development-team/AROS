#ifndef _COMPLEX_H_
#define _COMPLEX_H_
/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Standard C Library: C99 complex.h
*/

#include <aros/system.h>

/* GNU C provides an implementation of the complex type */
#ifdef __GNUC__
#   define  _Complex	__complex__
#   define  _Complex_I	1.0fi
#else
#error Unsupported compiler in complex.h
#endif

#define complex		_Complex
#define I		_Complex_I

__BEGIN_DECLS

//NOT IMPL double complex cacos(double complex z);
//NOT IMPL float complex cacosf(float complex z);
//NOT IMPL long double complex cacosl(long double complex z);
//NOT IMPL double complex casin(double complex z);
//NOT IMPL float complex casinf(float complex z);
//NOT IMPL long double complex casinl(long double complex z);
//NOT IMPL double complex catan(double complex z);
//NOT IMPL float complex catanf(float complex z);
//NOT IMPL long double complex catanl(long double complex z);
//NOT IMPL double complex ccos(double complex z);
//NOT IMPL float complex ccosf(float complex z);
//NOT IMPL long double complex ccosl(long double complex z);
//NOT IMPL double complex csin(double complex z);
//NOT IMPL float complex csinf(float complex z);
//NOT IMPL long double complex csinl(long double complex z);
//NOT IMPL double complex ctan(double complex z);
//NOT IMPL float complex ctanf(float complex z);
//NOT IMPL long double complex ctanl(long double complex z);
//NOT IMPL double complex cacosh(double complex z);
//NOT IMPL float complex cacoshf(float complex z);
//NOT IMPL long double complex cacoshl(long double complex z);
//NOT IMPL double complex casinh(double complex z);
//NOT IMPL float complex casinhf(float complex z);
//NOT IMPL long double complex casinhl(long double complex z);
//NOT IMPL double complex catanh(double complex z);
//NOT IMPL float complex catanhf(float complex z);
//NOT IMPL long double complex catanhl(long double complex z);
//NOT IMPL double complex ccosh(double complex z);
//NOT IMPL float complex ccoshf(float complex z);
//NOT IMPL long double complex ccoshl(long double complex z);
//NOT IMPL double complex csinh(double complex z);
//NOT IMPL float complex csinhf(float complex z);
//NOT IMPL long double complex csinhl(long double complex z);
//NOT IMPL double complex ctanh(double complex z);
//NOT IMPL float complex ctanhf(float complex z);
//NOT IMPL long double complex ctanhl(long double complex z);
//NOT IMPL double complex cexp(double complex z);
//NOT IMPL float complex cexpf(float complex z);
//NOT IMPL long double complex cexpl(long double complex z);
//NOT IMPL double complex clog(double complex z);
//NOT IMPL float complex clogf(float complex z);
//NOT IMPL long double complex clogl(long double complex z);
double cabs(double complex z);
float cabsf(float complex z);
//NOT IMPL long double cabsl(long double complex z);
//NOT IMPL double complex cpow(double complex x, double complex y);
//NOT IMPL float complex cpowf(float complex x, float complex y);
//NOT IMPL long double complex cpowl(long double complex x, long double complex y);
//NOT IMPL double complex csqrt(double complex z);
//NOT IMPL float complex csqrtf(float complex z);
//NOT IMPL long double complex csqrtl(long double complex z);
//NOT IMPL double carg(double complex z);
//NOT IMPL float cargf(float complex z);
//NOT IMPL long double cargl(long double complex z);
double cimag(double complex z);
float cimagf(float complex z);
long double cimagl(long double complex z);
double complex conj(double complex z);
float complex conjf(float complex z);
long double complex conjl(long double complex z);
//NOT IMPL double complex cproj(double complex z);
//NOT IMPL float complex cprojf(float complex z);
//NOT IMPL long double complex cprojl(long double complex z);
double creal(double complex z);
float crealf(float complex z);
long double creall(long double complex z);

__END_DECLS

#ifdef __GNUC__
#define	cimag(z)	(__imag__ (z))
#define cimagf(z)	(__imag__ (z))
#define cimagl(z)	(__imag__ (z))

#define creal(z)	(__real__ (z))
#define crealf(z)	(__real__ (z))
#define creall(z)	(__real__ (z))
#endif

#endif /* _COMPLEX_H_ */

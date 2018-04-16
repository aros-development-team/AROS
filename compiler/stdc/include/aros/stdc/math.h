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

/*
 * from: @(#)fdlibm.h 5.1 93/09/24
 * $FreeBSD: src/lib/msun/src/math.h,v 1.62 2007/01/07 07:54:21 das Exp $
 */

#ifndef _MATH_H_
#define _MATH_H_

#include <aros/system.h>
#include <inttypes.h>
#include <limits.h>

#ifndef __GNUC_PREREQ__
#define __GNUC_PREREQ__ __GNUC_PREREQ
#endif

/*
 * ANSI/POSIX
 */
extern const union __infinity_un {
	unsigned char	__uc[8];
	double		__ud;
} __infinity;

extern const union __nan_un {
	unsigned char	__uc[sizeof(float)];
	float		__uf;
} __nan;

#if __GNUC_PREREQ__(3, 3) || (defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 800)
#define	__MATH_BUILTIN_CONSTANTS
#endif

#if __GNUC_PREREQ__(3, 0) && !defined(__INTEL_COMPILER)
#define	__MATH_BUILTIN_RELOPS
#endif

#ifdef __MATH_BUILTIN_CONSTANTS
#define	HUGE_VAL	__builtin_huge_val()
#else
#define	HUGE_VAL	(__infinity.__ud)
#endif

/* #if __ISO_C_VISIBLE >= 1999 */
#define        FP_ILOGB0       (-INT_MAX)
#define        FP_ILOGBNAN     INT_MAX

#ifdef __MATH_BUILTIN_CONSTANTS
#define	HUGE_VALF	__builtin_huge_valf()
#define	HUGE_VALL	__builtin_huge_vall()
#define	INFINITY	__builtin_inf()
#define	NAN		__builtin_nan("")
#else
#define	HUGE_VALF	(float)HUGE_VAL
#define	HUGE_VALL	(long double)HUGE_VAL
#define	INFINITY	HUGE_VALF
#define	NAN		(__nan.__uf)
#endif /* __MATH_BUILTIN_CONSTANTS */

#define	MATH_ERRNO	1
#define	MATH_ERREXCEPT	2
#ifdef __mc68000__
#define	math_errhandling	MATH_ERRNO
#else
#define	math_errhandling	MATH_ERREXCEPT
#endif

/* XXX We need a <machine/math.h>. */
#if defined(__ia64__) || defined(__sparc64__)
#define	FP_FAST_FMA
#endif
#ifdef __ia64__
#define	FP_FAST_FMAL
#endif
#define	FP_FAST_FMAF

/* Symbolic constants to classify floating point numbers. */
#define	FP_INFINITE	0x01
#define	FP_NAN		0x02
#define	FP_NORMAL	0x04
#define	FP_SUBNORMAL	0x08
#define	FP_ZERO		0x10

#if __GNUC_PREREQ__(3, 1) && !defined(__cplusplus)
#define	__fp_type_select(x, f, d, ld) __builtin_choose_expr(            \
    __builtin_types_compatible_p(__typeof(x), long double), ld(x),      \
    __builtin_choose_expr(                                              \
    __builtin_types_compatible_p(__typeof(x), double), d(x),            \
    __builtin_choose_expr(                                              \
    __builtin_types_compatible_p(__typeof(x), float), f(x), (void)0)))
#else
#define	 __fp_type_select(x, f, d, ld)                                  \
    ((sizeof(x) == sizeof(float)) ? f(x)                                \
    : (sizeof(x) == sizeof(double)) ? d(x)                              \
    : ld(x))
#endif

#define	fpclassify(x) \
    __fp_type_select(x, __fpclassifyf, __fpclassifyd, __fpclassifyl)
#define	isfinite(x) __fp_type_select(x, __isfinitef, __isfinite, __isfinitel)
#define	isinf(x) __fp_type_select(x, __isinff, __isinf, __isinfl)
#if !defined(NOLIBINLINE)
#define	isnan(x) \
    __fp_type_select(x, __inline_isnanf, __inline_isnan, __inline_isnanl)
#else
#define	isnan(x) \
    __fp_type_select(x, __isnanf, __isnan, __isnanl)
#endif
#define	isnormal(x) __fp_type_select(x, __isnormalf, __isnormal, __isnormall)

#ifdef __MATH_BUILTIN_RELOPS
#define	isgreater(x, y)		__builtin_isgreater((x), (y))
#define	isgreaterequal(x, y)	__builtin_isgreaterequal((x), (y))
#define	isless(x, y)		__builtin_isless((x), (y))
#define	islessequal(x, y)	__builtin_islessequal((x), (y))
#define	islessgreater(x, y)	__builtin_islessgreater((x), (y))
#define	isunordered(x, y)	__builtin_isunordered((x), (y))
#else
#define	isgreater(x, y)		(!isunordered((x), (y)) && (x) > (y))
#define	isgreaterequal(x, y)	(!isunordered((x), (y)) && (x) >= (y))
#define	isless(x, y)		(!isunordered((x), (y)) && (x) < (y))
#define	islessequal(x, y)	(!isunordered((x), (y)) && (x) <= (y))
#define	islessgreater(x, y)	(!isunordered((x), (y)) && \
					((x) > (y) || (y) > (x)))
#define	isunordered(x, y)	(isnan(x) || isnan(y))
#endif /* __MATH_BUILTIN_RELOPS */

#define	signbit(x) __fp_type_select(x, __signbitf, __signbit, __signbitl)

#if FLT_EVAL_METHOD <= 0
typedef	double double_t;
typedef	float float_t;
#elif FLT_EVAL_METHOD == 1
typedef double double_t;
typedef double float_t;
#elif FLT_EVAL_METHOD == 2
typedef long double double_t;
typedef long double float_t;
#endif
/* #endif __ISO_C_VISIBLE >= 1999 */

/*
 * XOPEN/SVID
 */
/* #if __BSD_VISIBLE || __XSI_VISIBLE */ 
#define	M_E		2.7182818284590452354	/* e */
#define	M_LOG2E		1.4426950408889634074	/* log 2e */
#define	M_LOG10E	0.43429448190325182765	/* log 10e */
#define	M_LN2		0.69314718055994530942	/* log e2 */
#define	M_LN10		2.30258509299404568402	/* log e10 */
#define	M_PI		3.14159265358979323846	/* pi */
#define	M_PI_2		1.57079632679489661923	/* pi/2 */
#define	M_PI_4		0.78539816339744830962	/* pi/4 */
#define	M_1_PI		0.31830988618379067154	/* 1/pi */
#define	M_2_PI		0.63661977236758134308	/* 2/pi */
#define	M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#define	M_SQRT2		1.41421356237309504880	/* sqrt(2) */
#define	M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

#define	MAXFLOAT	((float)3.40282346638528860e+38)

/* This is not fully standard compliant, code should normally be able
 * to just declare extern int signgam; in the program but this is not
 * supported.
 */
int *__stdc_getsigngamptr(void);
#ifndef signgam
#define signgam (*__stdc_getsigngamptr())
#endif
/* #endif __BSD_VISIBLE || __XSI_VISIBLE */

/* #if __BSD_VISIBLE */
#define	HUGE		MAXFLOAT
/* #endif __BSD_VISIBLE */

/*
 * Most of these functions depend on the rounding mode and have the side
 * effect of raising floating-point exceptions, so they are not declared
 * as __pure2.  In C99, FENV_ACCESS affects the purity of these functions.
 */
__BEGIN_DECLS
/*
 * Internal support functions
 */
int	__fpclassifyd(double) __pure2;
int	__fpclassifyf(float) __pure2;
int	__fpclassifyl(long double) __pure2;
int	__isfinitef(float) __pure2;
int	__isfinite(double) __pure2;
int	__isfinitel(long double) __pure2;
int	__isinf(double) __pure2;
int	__isinff(float) __pure2;
int	__isinfl(long double) __pure2;
int	__isnormal(double) __pure2;
int	__isnormalf(float) __pure2;
int	__isnormall(long double) __pure2;
int	__signbit(double) __pure2;
int	__signbitf(float) __pure2;
int	__signbitl(long double) __pure2;
static __inline int
__inline_isnan(__const double __x)
{
    return (__x != __x);
}

static __inline int
__inline_isnanf(__const float __x)
{
    return (__x != __x);
}

static __inline int
__inline_isnanl(__const long double __x)

{
    return (__x != __x);
}
int     __isnan(double) __pure2;
int     __isnanf(float) __pure2;
int	__isnanl(long double) __pure2;

/*
 * C99 double versions of functions
 */
double	acos(double);
double	asin(double);
double	atan(double);
double	atan2(double, double);
double	cos(double);
double	sin(double);
double	tan(double);

double	acosh(double);
double	asinh(double);
double	atanh(double);
double	cosh(double);
double	sinh(double);
double	tanh(double);

double	exp(double);
double	exp2(double);
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
double	sqrt(double);

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

/*
 * Version 2 of the Single UNIX Specification (UNIX98) defined isnan() and
 * isinf() as functions taking double.  C99, and the subsequent POSIX revisions
 * (SUSv3, POSIX.1-2001, define it as a macro that accepts any real floating
 * point type.  If we are targeting SUSv2 and C99 or C11 (or C++11) then we
 * expose the newer definition, assuming that the language spec takes
 * precedence over the operating system interface spec.
 */
/* #if __XSI_VISIBLE > 0 && __XSI_VISIBLE < 600 && __ISO_C_VISIBLE < 1999
#undef isinf
#undef isnan */
int	(isinf)(double) __pure2;
int	(isnan)(double) __pure2;
/* #endif */

/*
 * double functions extensions; mostly from BSD
 */
double	j0(double);
double	j1(double);
double	jn(int, double);
double	scalb(double, double);
double	y0(double);
double	y1(double);
double	yn(int, double);
double	gamma_r(double, int *);
double	lgamma_r(double, int *);
void	sincos(double x, double *sin, double *cos);

/* IEEE Test Vector */
double	significand(double);

/*
 * C99 float versions of functions
 */
float	acosf(float);
float	asinf(float);
float	atanf(float);
float	atan2f(float, float);
float	cosf(float);
float	sinf(float);
float	tanf(float);

float	acoshf(float);
float	asinhf(float);
float	atanhf(float);
float	coshf(float);
float	sinhf(float);
float	tanhf(float);

float	expf(float);
float	exp2f(float);
float	expm1f(float);
float	frexpf(float, int *);	/* fundamentally !__pure2 */
int	ilogbf(float) __pure2;
float	ldexpf(float, int);
float	logf(float);
float	log10f(float);
float	log1pf(float);
float	log2f(float);
float	logbf(float);
float	modff(float, float *);	/* fundamentally !__pure2 */
float	scalbnf(float, int);
float	scalblnf(float, long);

float	cbrtf(float);
float	fabsf(float) __pure2;
float	hypotf(float, float);
float	powf(float, float);
float	sqrtf(float);

float	erff(float);
float	erfcf(float);
float	lgammaf(float);
float	tgammaf(float);

float	ceilf(float);
float	floorf(float);
float	nearbyintf(float);
float	rintf(float);
long	lrintf(float);
long long llrintf(float);
float	roundf(float);
long	lroundf(float);
long long llroundf(float);
float	truncf(float);

float	fmodf(float, float);
float	remainderf(float, float);
float	remquof(float, float, int *);

float	copysignf(float, float) __pure2;
float   nanf(const char *tagp);
float	nextafterf(float, float);
float	nexttowardf(float, long double);

float	fdimf(float, float);
float	fmaxf(float, float) __pure2;
float	fminf(float, float) __pure2;

float	fmaf(float, float, float);

/* aliases */
float	gammaf(float);
float	dremf(float, float);
int	finitef(float) __pure2;
int	isinff(float) __pure2;
int	isnanf(float) __pure2;

/*
 * float functions extensions; mostly from BSD
 */
float	j0f(float);
float	j1f(float);
float	jnf(int, float);
float	scalbf(float, float);
float	y0f(float);
float	y1f(float);
float	ynf(int, float);
float	gammaf_r(float, int *);
float	lgammaf_r(float, int *);
void	sincosf(float x, float *sin, float *cos);

/* IEEE Test Vector */
float	significandf(float);

/*
 * C99 long double versions of functions
 */
long double	acosl(long double);
long double	asinl(long double);
long double	atanl(long double);
long double	atan2l(long double, long double);
long double	cosl(long double);
long double	sinl(long double);
long double	tanl(long double);

long double	acoshl(long double);
long double	asinhl(long double);
long double	atanhl(long double);
long double	coshl(long double);
long double	sinhl(long double);
long double	tanhl(long double);

long double	expl(long double);
long double	exp2l(long double);
long double	expm1l(long double);
long double	frexpl(long double value, int *); /* fundamentally !__pure2 */
int		ilogbl(long double) __pure2;
long double	ldexpl(long double, int);
long double	logl(long double);
long double	log10l(long double);
long double	log1pl(long double);
long double	log2l(long double);
long double	logbl(long double);
long double	modfl(long double, long double *); /* fundamentally !__pure2 */
long double	scalbnl(long double, int);
long double	scalblnl(long double, long);

long double	cbrtl(long double);
long double	fabsl(long double) __pure2;
long double	hypotl(long double, long double);
long double	powl(long double, long double);
long double	sqrtl(long double);

long double	erfl(long double);
long double	erfcl(long double);
long double	lgammal(long double);
long double	tgammal(long double);

long double	ceill(long double);
long double	floorl(long double);
long double	nearbyintl(long double);
long double	rintl(long double);
long		lrintl(long double);
long long	llrintl(long double);
long double	roundl(long double);
long		lroundl(long double);
long long	llroundl(long double);
long double	truncl(long double);

long double	fmodl(long double, long double);
long double	remainderl(long double, long double);
long double	remquol(long double, long double, int *);

long double	copysignl(long double, long double) __pure2;
long double     nanl(const char *tagp);
long double	nextafterl(long double, long double);
long double	nexttowardl(long double, long double);

long double	fdiml(long double, long double);
long double	fmaxl(long double, long double) __pure2;
long double	fminl(long double, long double) __pure2;

long double	fmal(long double, long double, long double);

/* aliases */
long double 	gammal(long double);
long double	dreml(long double, long double);
int	finitel(long double) __pure2;
int	isinfl(long double) __pure2;
int	isnanl(long double) __pure2;

/*
 * long double extensions; mostly from BSD
 */
//NOT IMPL long double	j0l(long double);
//NOT IMPL long double	j1l(long double);
//NOT IMPL long double	jnl(int, long double);
//NOT IMPL long double	scalbl(long double, long double);
//NOT IMPL long double	y0l(long double);
//NOT IMPL long double	y1l(long double);
//NOT IMPL long double	ynl(int, long double);
//NOT IMPL long double	gammal_r(long double, int *);
long double	lgammal_r(long double, int *);
void	sincosl(long double x, long double *sin, long double *cos);

/* IEEE Test Vector */
//NOT IMPL long double	significandl(long double);

__END_DECLS

#endif /* !_MATH_H_ */

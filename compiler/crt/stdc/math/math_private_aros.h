
#ifndef _MATH_PRIVATE_AROS_H_
#define _MATH_PRIVATE_AROS_H_

/* IEEE754 style elementary function declarations */

extern double __ieee754_acos(double x);
extern double __ieee754_acosh(double x);
extern double __ieee754_asin(double x);
extern double __ieee754_atan(double x);
extern double __ieee754_atan2(double y, double x);
extern double __ieee754_atanh(double x);
extern double __ieee754_cbrt(double x);
extern double __ieee754_cos(double x);
extern double __ieee754_cosh(double x);
extern double __ieee754_erf(double x);
extern double __ieee754_erfc(double x);
extern double __ieee754_exp(double x);
extern double __ieee754_fmod(double x, double y);
extern double __ieee754_hypot(double x, double y);
extern double __ieee754_j0(double x);
extern double __ieee754_j1(double x);
extern double __ieee754_jn(int n, double x);
extern double __ieee754_lgamma(double x);
extern double __ieee754_log(double x);
extern double __ieee754_log10(double x);
extern double __ieee754_log1p(double x);
extern double __ieee754_log2(double x);
extern double __ieee754_modf(double x, double *iptr);
extern double __ieee754_pow(double x, double y);
extern double __ieee754_remquo(double x, double y, int *quo);
extern double __ieee754_sin(double x);
extern double __ieee754_sinh(double x);
extern double __ieee754_sqrt(double x);
extern double __ieee754_tan(double x);
extern double __ieee754_tanh(double x);
extern double __ieee754_tgamma(double x);
extern double __ieee754_y0(double x);
extern double __ieee754_y1(double x);
extern double __ieee754_yn(int n, double x);

#if LDBL_MANT_DIG != DBL_MANT_DIG
extern long double __ieee754_acosl(long double x);
extern long double __ieee754_acoshl(long double x);
extern long double __ieee754_asinl(long double x);
extern long double __ieee754_atanl(long double x);
extern long double __ieee754_atan2l(long double y, long double x);
extern long double __ieee754_atanhl(long double x);
extern long double __ieee754_cbrtl(long double x);
extern long double __ieee754_cosl(long double x);
extern long double __ieee754_coshl(long double x);
extern long double __ieee754_erfcl(long double x);
extern long double __ieee754_erfl(long double x);
extern long double __ieee754_expl(long double x);
extern long double __ieee754_fmodl(long double x, long double y);
extern long double __ieee754_hypotl(long double x, long double y);
extern long double __ieee754_lgammal(long double x);
extern long double __ieee754_log10l(long double x);
extern long double __ieee754_log1pl(long double x);
extern long double __ieee754_log2l(long double x);
extern long double __ieee754_logl(long double x);
extern long double __ieee754_modfl(long double x, long double *iptr);
extern long double __ieee754_powl(long double x, long double y);
extern long double __ieee754_remquol(long double x, long double y, int *quo);
extern long double __ieee754_sinl(long double x);
extern long double __ieee754_sinhl(long double x);
extern long double __ieee754_sqrtl(long double x);
extern long double __ieee754_tanl(long double x);
extern long double __ieee754_tanhl(long double x);
extern long double __ieee754_tgammal(long double x);
#endif


#ifdef STDC_STATIC
#define	STDC_SETMATHERRNO(x)
#define STDC_RAISEMATHEXCPT(x)
#else
#define	STDC_SETMATHERRNO(x) errno=x;
#define STDC_RAISEMATHEXCPT(x) feraiseexcept(x);
#endif

/* Helper to handle NaN/8 transparently */
#define FORWARD_IF_NAN_OR_INF(func, x) \
    do { \
        if (!isfinite(x)) \
            return __ieee754_##func(x); \
    } while (0)

#define FORWARD_IF_NAN_OR_INF2(func, a, b) \
    do { \
        if (!isfinite(a) || !isfinite(b)) \
            return __ieee754_##func(a, b); \
    } while (0)

#endif /* _MATH_PRIVATE_AROS_H_ */
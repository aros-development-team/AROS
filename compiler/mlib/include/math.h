#ifndef	_MATH_H
#define	_MATH_H

extern int signgam;

/* Prototypes */
extern double __ieee754_acos(double x);
extern double __ieee754_acosh(double x);
extern double __ieee754_asin(double x);
extern double __ieee754_atan2(double y, double x);
extern double __ieee754_atanh(double x);
extern double __ieee754_cosh(double x);
extern double __ieee754_exp(double x);
extern double __ieee754_fmod(double x, double y);
extern double __ieee754_gamma(double x);
extern double __ieee754_gamma_r(double x, int *signgamp);
extern double __ieee754_hypot(double x, double y);
extern double __ieee754_j0(double x) ;
extern double __ieee754_y0(double x) ;
extern double __ieee754_j1(double x) ;
extern double __ieee754_y1(double x) ;
extern double __ieee754_jn(int n, double x);
extern double __ieee754_yn(int n, double x) ;
extern double __ieee754_lgamma(double x);
extern double __ieee754_lgamma_r(double x, int *signgamp);
extern double __ieee754_log(double x);
extern double __ieee754_log10(double x);
extern double __ieee754_pow(double x, double y);
extern int __ieee754_rem_pio2(double x, double *y);
extern double __ieee754_remainder(double x, double p);
#ifdef _SCALB_INT
extern double __ieee754_scalb(double x, int fn);
#else
extern double __ieee754_scalb(double x, double fn);
#endif
extern double __ieee754_sinh(double x);
extern double __ieee754_sqrt(double x);
extern double __kernel_cos(double x, double y);
extern int __kernel_rem_pio2(double *x, double *y, int e0, int nx, int prec, const int *ipio2) ;
extern double __kernel_sin(double x, double y, int iy);
extern double __kernel_standard(double x, double y, int type) ;
extern double __kernel_tan(double x, double y, int iy);
extern double asinh(double x);
extern double atan(double x);
extern double cbrt(double x) ;
extern double ceil(double x);
extern double copysign(double x, double y);
extern double cos(double x);
extern double erf(double x) ;
extern double erfc(double x) ;
extern double expm1(double x);
extern double fabs(double x);
extern int finite(double x);
extern double floor(double x);
extern double frexp(double x, int *eptr);
extern int ilogb(double x);
extern int isnan(double x);
extern double ldexp(double value, int exp);
extern double log1p(double x);
extern double logb(double x);
extern double modf(double x, double *iptr);
extern double nextafter(double x, double y);
extern double rint(double x);
extern double scalbn (double x, int n);
extern double sin(double x);
extern double tan(double x);
extern double tanh(double x);
extern double acos(double x);
extern double acosh(double x);
extern double asin(double x);
extern double atan2(double y, double x);
extern double atanh(double x);
extern double cosh(double x);
extern double exp(double x);
extern double fmod(double x, double y);
extern double gamma(double x);
extern double gamma_r(double x, int *signgamp);
extern double hypot(double x, double y);
extern double j0(double x);
extern double y0(double x);
extern double j1(double x);
extern double y1(double x);
extern double jn(int n, double x);
extern double yn(int n, double x);
extern double lgamma(double x);
extern double lgamma_r(double x, int *signgamp);
extern double log(double x);
extern double log10(double x);
extern double pow(double x, double y);
extern double remainder(double x, double y);
#ifdef _SCALB_INT
extern double scalb(double x, int fn);
#else
extern double scalb(double x, double fn);
#endif
extern double sinh(double x);
extern double sqrt(double x);

#include <float.h> /* For DBL_MAX */

#ifndef HUGE
#define HUGE		DBL_MAX
#endif

#ifndef HUGE_VAL
#define HUGE_VAL	DBL_MAX
#endif

#ifndef M_E
#define M_E         2.7182818284590452354	/* e */
#endif
#ifndef M_LOG2E
#define M_LOG2E     1.4426950408889634074	/* log 2e */
#endif
#ifndef M_LOG10E
#define M_LOG10E    0.43429448190325182765	/* log 10e */
#endif
#ifndef M_LN2
#define M_LN2       0.69314718055994530942	/* log e2 */
#endif
#ifndef M_LN10
#define M_LN10      2.30258509299404568402	/* log e10 */
#endif
#ifndef M_PI
#define M_PI        3.14159265358979323846	/* pi */
#endif
#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923	/* pi/2 */
#endif
#ifndef M_1_PI
#define M_1_PI      0.31830988618379067154	/* 1/pi */
#endif
#ifndef M_PI_4
#define M_PI_4      0.78539816339744830962	/* pi/4 */
#endif
#ifndef M_2_PI
#define M_2_PI      0.63661977236758134308	/* 2/pi */
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI  1.12837916709551257390	/* 2/sqrt(pi) */
#endif
#ifndef M_SQRT2
#define M_SQRT2     1.41421356237309504880	/* sqrt(2) */
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2   0.70710678118654752440	/* 1/sqrt(2) */
#endif

#ifndef PI
#define PI  M_PI
#endif
#ifndef PI2
#define PI2  (M_PI + M_PI)
#endif


#endif /* _MATH_H */

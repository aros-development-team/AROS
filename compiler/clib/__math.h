#ifndef __MATH_H
#define __MATH_H

/* This is a partial copy of fdlibm.h from the fdlibm package by Sun */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#ifndef AROS_MACHINE_H
#   include <aros/machine.h>
#endif
#include <math.h>

#if AROS_BIG_ENDIAN
#   define __HI(x) *(int*)&x
#   define __LO(x) *(1+(int*)&x)
#   define __HIp(x) *(int*)x
#   define __LOp(x) *(1+(int*)x)
#else
#   define __HI(x) *(1+(int*)&x)
#   define __LO(x) *(int*)&x
#   define __HIp(x) *(1+(int*)x)
#   define __LOp(x) *(int*)x
#endif

/* Use non-standard matherr() and smart code */
#if !defined(__FreeBSD__)
#define _LIB_VERSION	0
#define _POSIX_ 	1
#define _SVID_		2
#define _IEEE_		3
#elif defined(__FreeBSD__)
#undef _LIB_VERSION
#define _LIB_VERSION 	0
#endif

/* We only include this section if we don't have glibc v2 or FreeBSD */
#if (!defined(__GLIBC__) || (__GLIBC__ < 2)) && !defined(__FreeBSD__)
struct exception
{
    int    type;
    char  *name;
    double arg1;
    double arg2;
    double retval;
};
#endif

/* exception.type */
#define DOMAIN		1
#define SING		2
#define OVERFLOW	3
#define UNDERFLOW	4
#define TLOSS		5
#define PLOSS		6

#if defined(__FreeBSD__)
#undef HUGE_VAL
#endif

#ifdef copysign
#  undef copysign
#endif

extern double __kernel_standard (double, double, int);
extern double scalbn		(double, int);
extern double copysign		(double x, double y);
extern double __ieee754_pow	(double, double);
extern double __ieee754_log10	(double);
extern double __ieee754_log	(double);
extern int    isinf		(double);
extern int    __isinf		(double);

#if 0 /* Unused */
extern int    matherr		(struct exception *);
#endif

#endif /* __MATH_H */

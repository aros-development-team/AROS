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
#define _LIB_VERSION	0
#define _POSIX_ 	1
#define _SVID_		2
#define _IEEE_		3

struct exception
{
    int    type;
    char  *name;
    double arg1;
    double arg2;
    double retval;
};

/* exception.type */
#define DOMAIN		1
#define SING		2
#define OVERFLOW	3
#define UNDERFLOW	4
#define TLOSS		5
#define PLOSS		6

extern double __kernel_standard (double, double, int);
extern double __ieee754_pow	(double, double);
extern int    matherr		(struct exception *);
extern double scalbn		(double, int);
extern double __ieee754_log10	(double);
extern double __ieee754_log	(double);


#endif /* __MATH_H */

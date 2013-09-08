/* e_gammaf.c -- float version of e_gamma.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
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

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/e_gammaf.c,v 1.6 2002/05/28 17:03:12 alfred Exp $";
#endif

/* __ieee754_gammaf(x)
 * Return the logarithm of the Gamma function of x.
 *
 * Method: call __ieee754_gammaf_r
 */

#include "math.h"
#include "math_private.h"

extern int signgam;

float
__ieee754_gammaf(float x)
{
	return __ieee754_gammaf_r(x,&signgam);
}

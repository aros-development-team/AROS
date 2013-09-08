
/* @(#)e_gamma_r.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 *
 */

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/e_gamma_r.c,v 1.7 2005/02/04 18:26:05 das Exp $";
#endif

/* __ieee754_gamma_r(x, signgamp)
 * Reentrant version of the logarithm of the Gamma function
 * with user provide pointer for the sign of Gamma(x).
 *
 * Method: See __ieee754_lgamma_r
 */

#include "math.h"
#include "math_private.h"

double
__ieee754_gamma_r(double x, int *signgamp)
{
	return __ieee754_lgamma_r(x,signgamp);
}

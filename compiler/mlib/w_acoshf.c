/* w_acoshf.c -- float version of w_acosh.c.
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
 *
 */

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/w_acoshf.c,v 1.5 1999/08/28 00:06:57 peter Exp $";
#endif

/*
 * wrapper acoshf(x)
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	float acoshf(float x)		/* wrapper acoshf */
#else
	float acoshf(x)			/* wrapper acoshf */
	float x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_acoshf(x);
#else
	float z;
	z = __ieee754_acoshf(x);
	if(_LIB_VERSION == _IEEE_ || isnanf(x)) return z;
	if(x<(float)1.0) {
		/* acosh(x<1) */
	        return (float)__kernel_standard((double)x,(double)x,129);
	} else
	    return z;
#endif
}

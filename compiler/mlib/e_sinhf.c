/* e_sinhf.c -- float version of e_sinh.c.
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/e_sinhf.c,v 1.8 2005/11/13 00:41:46 bde Exp $";
#endif

#include "math.h"
#include "math_private.h"

static const float one = 1.0, shuge = 1.0e37;

float
__ieee754_sinhf(float x)
{
	float t,w,h;
	int32_t ix,jx;

	GET_FLOAT_WORD(jx,x);
	ix = jx&0x7fffffff;

    /* x is INF or NaN */
	if(ix>=0x7f800000) return x+x;

	h = 0.5;
	if (jx<0) h = -h;
    /* |x| in [0,9], return sign(x)*0.5*(E+E/(E+1))) */
	if (ix < 0x41100000) {		/* |x|<9 */
	    if (ix<0x39800000) 		/* |x|<2**-12 */
		if(shuge+x>one) return x;/* sinh(tiny) = tiny with inexact */
	    t = expm1f(fabsf(x));
	    if(ix<0x3f800000) return h*((float)2.0*t-t*t/(t+one));
	    return h*(t+t/(t+one));
	}

    /* |x| in [9, logf(maxfloat)] return 0.5*exp(|x|) */
	if (ix < 0x42b17217)  return h*__ieee754_expf(fabsf(x));

    /* |x| in [logf(maxfloat), overflowthresold] */
	if (ix<=0x42b2d4fc) {
	    w = __ieee754_expf((float)0.5*fabsf(x));
	    t = h*w;
	    return t*w;
	}

    /* |x| > overflowthresold, sinh(x) overflow */
	return x*shuge;
}

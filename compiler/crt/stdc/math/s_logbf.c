/* s_logbf.c -- float version of s_logb.c.
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_logbf.c,v 1.9 2008/02/22 02:30:35 das Exp $";
#endif

#include "math.h"
#include "math_private.h"

static const float
two25 = 3.355443200e+07;		/* 0x4c000000 */

/****************************************************************************
    NAME */
        float logbf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Return the base-two exponent of the magnitude of x
        as a floating-point value.

    INPUTS
        x - Float value to examine.

    RESULT
        For finite nonzero x, e such that 2^e <= |x| < 2^(e+1).
        Zero returns negative infinity, infinity returns positive
        infinity, and NaN returns NaN.

    NOTES
        Include <math.h>. The zero path divides by zero and can
        raise FE_DIVBYZERO. A floating-point trap may prevent
        return. Unlike ilogbf(), special inputs produce
        floating-point values rather than integer sentinels.

    SEE ALSO
        logb(), logbl(), ilogbf(), frexpf()

*****************************************************************************/
{
	int32_t ix;
	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;			/* high |x| */
	if(ix==0) return (float)-1.0/fabsf(x);
	if(ix>=0x7f800000) return x*x;
	if(ix<0x00800000) {
		x *= two25;		 /* convert subnormal x to normal */
		GET_FLOAT_WORD(ix,x);
		ix &= 0x7fffffff;
		return (float) ((ix>>23)-127-25);
	} else
		return (float) ((ix>>23)-127);
}

/* s_ilogbf.c -- float version of s_ilogb.c.
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_ilogbf.c,v 1.8 2008/02/22 02:30:35 das Exp $";
#endif

#include <limits.h>

#include "math.h"
#include "math_private.h"

/****************************************************************************
    NAME */
        int ilogbf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Return the binary exponent of |x| as an integer.

    INPUTS
        x - Float value to classify.

    RESULT
        The integer exponent e such that 2^e <= |x| < 2^(e+1)
        for finite nonzero x. Zero returns FP_ILOGB0; NaN returns
        FP_ILOGBNAN; infinity returns INT_MAX. In AROS,
        FP_ILOGBNAN equals INT_MAX, so these results are ambiguous.

    NOTES
        Include <math.h>. Use isnan() or isinf() on the input to
        distinguish NaN from infinity. Unlike logbf(), the
        result is an integer sentinel for exceptional inputs.

    BUGS
        The subnormal scan shifts a signed 32-bit integer into its
        sign bit, an operation with undefined behavior in C.
        Subnormal results may depend on the compiler.

    SEE ALSO
        ilogb(), ilogbl(), logbf(), frexpf()

*****************************************************************************/
{
	int32_t hx,ix;

	GET_FLOAT_WORD(hx,x);
	hx &= 0x7fffffff;
	if(hx<0x00800000) {
	    if(hx==0)
		return FP_ILOGB0;
	    else			/* subnormal x */
	        for (ix = -126,hx<<=8; hx>0; hx<<=1) ix -=1;
	    return ix;
	}
	else if (hx<0x7f800000) return (hx>>23)-127;
	else if (hx>0x7f800000) return FP_ILOGBNAN;
	else return INT_MAX;
}

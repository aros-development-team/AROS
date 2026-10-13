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
        Extract the binary exponent of x as an int. For finite nonzero
        x, the exponent e satisfies 2^e <= |x| < 2^(e+1). The sign of x
        does not affect e. Subnormal inputs require a significand scan;
        see BUGS for implementation limitations on that path.

    INPUTS
        x - Float value whose exponent is required.

    RESULT
        For a finite nonzero value, the binary exponent. For example,
        12 and -12 yield 3; 0.5 yields -1. Both signs of zero return
        FP_ILOGB0. NaN returns FP_ILOGBNAN; either infinity returns
        INT_MAX. These are integer sentinels, not valid finite exponents.

        AROS defines FP_ILOGB0 as -INT_MAX and FP_ILOGBNAN as INT_MAX.
        The return value alone cannot distinguish NaN from infinity.

    NOTES
        Include <math.h>; <limits.h> declares INT_MAX. This function
        returns an integer exponent, not the logarithm of x. It differs
        from frexpf(): for finite nonzero x, its exponent is one less
        than the exponent of the fraction returned by that function.

        The implementation reads the floating-point representation and
        uses integer operations. It does not set or clear errno and has
        no explicit floating-point exception-raising operation. Do not
        use errno or an assumed FE_INVALID signal to distinguish the
        special return cases. Classify x when that distinction matters.
        Signaling-NaN handling and trap behavior are target-dependent.

        No application storage is modified or retained. The function
        allocates nothing and performs no I/O or waiting.

    BUGS
        The subnormal scan left-shifts a signed int32_t until its sign
        bit becomes set. Such a shift has undefined behavior in C when
        the result is not representable. Consequently, the mathematical
        subnormal result is not a portable guarantee of this source;
        compiler optimization may affect the observed behavior.

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

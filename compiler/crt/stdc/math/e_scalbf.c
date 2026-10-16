/* e_scalbf.c -- float version of e_scalb.c.
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/e_scalbf.c,v 1.12 2006/07/05 17:34:57 bde Exp $";
#endif

#include "math.h"
#include "math_private.h"

/****************************************************************************
    NAME
        float scalbf(

    SYNOPSIS
        float x, float fn)

    FUNCTION
        Scale x by 2 raised to the integral value fn.

    INPUTS
        x - Float value to scale.
        fn - Exponent, supplied as a float. Finite values must be
             integral. See BUGS for negative fractional exponents.

    RESULT
        The scaled value in float. Exact representable results
        require no rounding; inexact subnormal results and range
        limits depend on the current rounding direction.

        For finite integral fn, signed zero and infinity retain
        the sign of x. A NaN argument produces a NaN. A finite
        nonintegral fn produces a NaN, subject to BUGS.

        For fn equal to positive infinity, returns x * infinity.
        For fn equal to negative infinity, returns x / infinity.
        Thus zero with positive infinity, or infinite x with
        negative infinity, produces a NaN.

    NOTES
        Include <math.h>. Prefer scalbnf() for an int exponent
        or scalblnf() for a long exponent.

        The function does not assign errno. Invalid arguments and
        range errors can raise floating-point exceptions; the
        integrality check can also raise inexact. Enabled traps
        may prevent return.

    BUGS
        For -1 < fn < 0, the integrality check invokes undefined
        C behavior. Do not rely on a NaN result for these inputs.

    SEE ALSO
        scalb(), scalbnf(), scalblnf(), ldexpf()

*****************************************************************************/
#ifdef _SCALB_INT
float
__ieee754_scalbf(float x, int fn)
#else
float
__ieee754_scalbf(float x, float fn)
#endif
{
#ifdef _SCALB_INT
	return scalbnf(x,fn);
#else
	if ((__isnanf)(x)||(__isnanf)(fn)) return x*fn;
	if (!__isfinitef(fn)) {
	    if(fn>(float)0.0) return x*fn;
	    else       return x/(-fn);
	}
	if (rintf(fn)!=fn) return (fn-fn)/(fn-fn);
	if ( fn > (float)65000.0) return scalbnf(x, 65000);
	if (-fn > (float)65000.0) return scalbnf(x,-65000);
	return scalbnf(x,(int)fn);
#endif
}

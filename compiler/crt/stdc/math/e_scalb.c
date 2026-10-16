
/* @(#)e_scalb.c 1.3 95/01/18 */
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

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/e_scalb.c,v 1.12 2005/02/04 18:26:06 das Exp $";
#endif

/*
 * __ieee754_scalb(x, fn) is provide for
 * passing various standard test suite. One
 * should use scalbn() instead.
 */

#include "math.h"
#include "math_private.h"

/****************************************************************************
    NAME
        double scalb(

    SYNOPSIS
        double x, double fn)

    FUNCTION
        Scale x by 2 raised to the integral value fn.

    INPUTS
        x - Double value to scale.
        fn - Exponent, supplied as a double. Finite values must be
             integral. See BUGS for negative fractional exponents.

    RESULT
        The scaled value in double. Exact representable results
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
        Include <math.h>. Prefer scalbn() for an int exponent
        or scalbln() for a long exponent.

        The function does not assign errno. Invalid arguments and
        range errors can raise floating-point exceptions; the
        integrality check can also raise inexact. Enabled traps
        may prevent return.

    BUGS
        For -1 < fn < 0, the integrality check invokes undefined
        C behavior. Do not rely on a NaN result for these inputs.

    SEE ALSO
        scalbf(), scalbn(), scalbln(), ldexp()

*****************************************************************************/
#ifdef _SCALB_INT
double
__ieee754_scalb(double x, int fn)
#else
double
__ieee754_scalb(double x, double fn)
#endif
{
#ifdef _SCALB_INT
	return scalbn(x,fn);
#else
	if (isnan(x)||isnan(fn)) return x*fn;
	if (!__isfinite(fn)) {
	    if(fn>0.0) return x*fn;
	    else       return x/(-fn);
	}
	if (rint(fn)!=fn) return (fn-fn)/(fn-fn);
	if ( fn > 65000.0) return scalbn(x, 65000);
	if (-fn > 65000.0) return scalbn(x,-65000);
	return scalbn(x,(int)fn);
#endif
}

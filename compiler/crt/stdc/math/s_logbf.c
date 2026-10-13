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
        Extract the binary exponent of x and return it as a floating-point
        value. For finite nonzero x, the exponent e satisfies
        2^e <= |x| < 2^(e+1). The sign of x does not affect e. This is
        not a general logarithm: for example, 12 and -12 both yield 3.

    INPUTS
        x - Float value whose exponent is required. Signed zeros,
            infinities and NaNs have the special results described below.

    RESULT
        The binary exponent, represented exactly as float, for a
        finite nonzero input. A magnitude in [1, 2) yields positive zero.
        A subnormal input is normalized or scanned before its exponent
        is obtained.

        Either sign of zero yields negative infinity through division by
        zero. Either infinity yields positive infinity. NaN yields NaN.
        These value results assume nontrapping floating-point execution;
        an enabled trap may prevent the function from returning.

    NOTES
        Include <math.h>. Unlike ilogbf(), this function returns a
        floating-point value and uses infinities and NaNs for special
        inputs instead of integer sentinels. For finite nonzero x, its
        exponent is one less than the exponent reported by frexpf().

        The zero path performs a floating-point division by zero. Under
        an IEEE floating-point environment this raises FE_DIVBYZERO.
        The nonfinite path evaluates x * x. Exception flags, signaling-
        NaN handling and enabled traps depend on the target and its
        floating-point environment. No NaN payload preservation is
        guaranteed.

        The implementation does not set or clear errno. It does not
        allocate, modify or retain application storage, perform I/O, or
        wait. Finite exponent results do not depend on the rounding
        direction under ordinary gradual-underflow operation.

        Subnormal inputs are multiplied by 2^25 before the exponent field
        is extracted; the returned exponent compensates for that scale.
        An environment that treats subnormal operands as zero is outside
        this value contract.

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

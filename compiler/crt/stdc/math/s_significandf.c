/* s_significandf.c -- float version of s_significand.c.
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_significandf.c,v 1.7 2002/05/28 18:15:04 alfred Exp $";
#endif

#include "math.h"
#include "math_private.h"

/****************************************************************************
    NAME
        float significandf(

    SYNOPSIS
        float x)

    FUNCTION
        Return the signed binary significand of x.

    INPUTS
        x - Float value to normalize. See BUGS for subnormal inputs.

    RESULT
        For finite nonzero x, the intended result is x scaled by
        2 raised to -ilogbf(x), with magnitude at least 1 and less
        than 2. Normal inputs produce this result exactly.

        Signed zero and infinity retain their sign. NaN produces
        a NaN.

    NOTES
        Include <math.h>. Unlike frexpf(), this function returns
        a significand with magnitude in [1, 2) and does not return
        an exponent. For example, 12.0f yields 1.5f and -0.5f
        yields -1.0f.

        The function does not assign errno. Signaling-NaN handling
        and exception delivery depend on the target.

    BUGS
        Subnormal inputs invoke undefined C behavior during
        exponent extraction. Their normalized result is not
        guaranteed.

    SEE ALSO
        frexpf(), ilogbf(), scalbf(), scalbnf()

*****************************************************************************/
float
significandf(float x)
{
	return __ieee754_scalbf(x,(float) -ilogbf(x));
}

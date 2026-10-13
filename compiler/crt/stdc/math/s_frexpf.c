/* s_frexpf.c -- float version of s_frexp.c.
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_frexpf.c,v 1.10 2008/02/22 02:30:35 das Exp $";
#endif

#include "math.h"
#include "math_private.h"

static const float
two25 =  3.3554432000e+07; /* 0x4c000000 */

/****************************************************************************
    NAME */
        float frexpf(

/*  SYNOPSIS */
        float x,
        int *eptr)

/*  FUNCTION
        Split x into a signed binary fraction and an integer exponent.
        For finite nonzero x, the returned fraction m and stored exponent e
        satisfy x = m * 2^e, with 0.5 <= |m| < 1. Subnormal inputs are
        normalized before the fraction and exponent are extracted.

    INPUTS
        x - Float value to decompose.
        eptr - Non-NULL pointer to a writable int for the exponent. Required
            even when x is zero or nonfinite. The pointer is not retained.

    RESULT
        The signed fraction in float precision. A zero input returns
        the same signed zero and stores exponent 0. An infinite or NaN
        input returns that value; see NOTES for the exponent output.
        There is no separate error sentinel.

    NOTES
        Include <math.h>. The exponent is a power of two, not a decimal
        exponent. For example, x = 12 yields fraction 0.75 and exponent 4.

        The call writes through eptr but retains no application storage,
        allocates nothing, performs no I/O or waiting, and does not set
        or clear errno. NULL is not a supported exponent output: this
        function does not provide a discard-output or size-query mode.

        This implementation initializes *eptr to 0 before checking for
        zero, infinity or NaN. Thus a nonfinite input also leaves exponent
        0. That value does not make a NaN or infinity a finite decomposition.

        No NaN payload, signaling-NaN or trap-behavior guarantee is made
        here. The value contract assumes the target's supported floating-
        point representation and ordinary subnormal handling.

    SEE ALSO
        frexp(), frexpl(), ldexpf()

*****************************************************************************/
{
	int32_t hx,ix;
	GET_FLOAT_WORD(hx,x);
	ix = 0x7fffffff&hx;
	*eptr = 0;
	if(ix>=0x7f800000||(ix==0)) return x;	/* 0,inf,nan */
	if (ix<0x00800000) {		/* subnormal */
	    x *= two25;
	    GET_FLOAT_WORD(hx,x);
	    ix = hx&0x7fffffff;
	    *eptr = -25;
	}
	*eptr += (ix>>23)-126;
	hx = (hx&0x807fffff)|0x3f000000;
	SET_FLOAT_WORD(x,hx);
	return x;
}

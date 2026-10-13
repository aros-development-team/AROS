/* s_rintf.c -- float version of s_rint.c.
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_rintf.c,v 1.12 2008/02/22 02:30:35 das Exp $";
#endif

#include <float.h>
#include <stdint.h>

#include "math.h"
#include "math_private.h"

static const float
TWO23[2]={
  8.3886080000e+06, /* 0x4b000000 */
 -8.3886080000e+06, /* 0xcb000000 */
};

/****************************************************************************
    NAME */
        float rintf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Round x to an integral floating-point value using the current
        rounding direction. Nearest rounding selects the even integer
        at a tie; upward, downward and toward-zero rounding use their
        respective directions. See BUGS for implementation limitations.

    INPUTS
        x - Float value to round.

    RESULT
        The rounded value in float, not a C integer conversion.
        Signed zeros retain their sign. Infinities remain infinite and
        NaNs produce NaNs. An enabled trap may prevent return.
        Already integral finite values are unchanged. There is no
        separate error sentinel.

    NOTES
        Include <math.h>; use <fenv.h> to configure rounding. Halfway
        cases differ from roundf(), which rounds away from zero. A
        later integer conversion still needs its own range check.

        Unlike nearbyintf(), this function does not save and restore
        the floating-point environment. Its rounding arithmetic can
        raise FE_INEXACT for nonintegral inputs under IEEE execution.
        Existing exception flags are not deliberately cleared. Enabled
        traps, signaling NaNs and payload handling are target-dependent.

        The implementation does not set or clear errno, allocate or
        retain application storage, perform I/O, or wait.

        The source adds and subtracts a signed 2^23 to round fractional
        values. STRICT_ASSIGN controls the precision of the intermediate
        sum. Values with magnitude below one receive an explicit sign
        correction so a rounded zero keeps the input sign.

    BUGS
        The small negative-input path evaluates sx << 31 with sx a
        signed int32_t equal to one. The shifted value is not representable
        in that type, so this is undefined C behavior. Observed results
        on one compiler do not establish portable behavior for that path.

    SEE ALSO
        rint(), rintl(), nearbyintf(), roundf()

*****************************************************************************/
{
	int32_t i0,j0,sx;
	float w,t;
	GET_FLOAT_WORD(i0,x);
	sx = (i0>>31)&1;
	j0 = ((i0>>23)&0xff)-0x7f;
	if(j0<23) {
	    if(j0<0) {
		if((i0&0x7fffffff)==0) return x;
		STRICT_ASSIGN(float,w,TWO23[sx]+x);
	        t =  w-TWO23[sx];
		GET_FLOAT_WORD(i0,t);
		SET_FLOAT_WORD(t,(i0&0x7fffffff)|(sx<<31));
	        return t;
	    }
	    STRICT_ASSIGN(float,w,TWO23[sx]+x);
	    return w-TWO23[sx];
	}
	if(j0==0x80) return x+x;	/* inf or NaN */
	else return x;			/* x is integral */
}

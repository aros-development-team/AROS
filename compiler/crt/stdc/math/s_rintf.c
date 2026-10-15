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
        Round x to an integral floating-point value using the
        current rounding direction.

    INPUTS
        x - Float value to round.

    RESULT
        The rounded float value. Nearest ties select the even
        integer; other modes follow the current rounding direction.
        Signed zeros and infinities keep their sign; NaNs remain NaNs.

    NOTES
        Include <math.h> and <fenv.h>. Unlike nearbyintf(),
        rounding may raise FE_INEXACT. Unlike roundf(), ties are
        not always rounded away from zero.

    BUGS
        The small negative-input path shifts a signed integer into
        its sign bit, which is undefined behavior in C. Results
        for that path may depend on the compiler.

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

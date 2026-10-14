/* s_nextafterf.c -- float version of s_nextafter.c.
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_nextafterf.c,v 1.11 2008/02/22 02:30:35 das Exp $";
#endif

#include "math.h"
#include "math_private.h"

/****************************************************************************
    NAME
        float nextafterf(

    SYNOPSIS
        float x, float y)

    FUNCTION
        Return the adjacent float value in the direction of y.

    INPUTS
        x - Starting value.
        y - Direction value, supplied as float.

    RESULT
        The adjacent value toward y, or y converted to float when
        x and y compare equal. Equal zeros return the sign of y.
        A zero starting value and nonzero direction select the smallest
        subnormal with the sign of y.

        If either argument is NaN, the result is NaN. Stepping from
        infinity toward a finite value returns the largest finite value
        with the same sign. See BUGS for outward steps at the finite limit.

    NOTES
        Include <math.h>. Subnormal results and steps to zero can raise
        FE_UNDERFLOW and FE_INEXACT. Outward steps at the finite limit
        can raise FE_OVERFLOW and FE_INEXACT. Enabled traps may prevent
        return. This function does not set errno.

    BUGS
        An outward step from the largest positive or negative finite
        value can return that same finite value under directed rounding,
        instead of the corresponding infinity.

    SEE ALSO
        nexttowardf(), nextafter()

*****************************************************************************/
float
nextafterf(float x, float y)
{
	volatile float t;
	int32_t hx,hy,ix,iy;

	GET_FLOAT_WORD(hx,x);
	GET_FLOAT_WORD(hy,y);
	ix = hx&0x7fffffff;		/* |x| */
	iy = hy&0x7fffffff;		/* |y| */

	if((ix>0x7f800000) ||   /* x is nan */
	   (iy>0x7f800000))     /* y is nan */
	   return x+y;
	if(x==y) return y;		/* x=y, return y */
	if(ix==0) {				/* x == 0 */
	    SET_FLOAT_WORD(x,(hy&0x80000000)|1);/* return +-minsubnormal */
	    t = x*x;
	    if(t==x) return t; else return x;	/* raise underflow flag */
	}
	if(hx>=0) {				/* x > 0 */
	    if(hx>hy) {				/* x > y, x -= ulp */
		hx -= 1;
	    } else {				/* x < y, x += ulp */
		hx += 1;
	    }
	} else {				/* x < 0 */
	    if(hy>=0||hx>hy){			/* x < y, x -= ulp */
		hx -= 1;
	    } else {				/* x > y, x += ulp */
		hx += 1;
	    }
	}
	hy = hx&0x7f800000;
	if(hy>=0x7f800000) return x+x;	/* overflow  */
	if(hy<0x00800000) {		/* underflow */
	    t = x*x;
	    if(t!=x) {		/* raise underflow flag */
	        SET_FLOAT_WORD(y,hx);
		return y;
	    }
	}
	SET_FLOAT_WORD(x,hx);
	return x;
}

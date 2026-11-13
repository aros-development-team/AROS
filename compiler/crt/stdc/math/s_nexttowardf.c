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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_nexttowardf.c,v 1.3 2011/02/10 07:38:38 das Exp $";
#endif

#include <float.h>

#include "fpmath.h"
#include "math.h"
#include "math_private.h"

#define	LDBL_INFNAN_EXP	(LDBL_MAX_EXP * 2 - 1)

/****************************************************************************
    NAME
        float nexttowardf(

    SYNOPSIS
        float x, long double y)

    FUNCTION
        Return the adjacent float value in the direction of y. The direction
        is determined using the full long double value of y, even when
        y would round to x if converted to float.

    INPUTS
        x - Starting value.
        y - Direction value, supplied as long double.

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

        Stepping from zero toward a negative value is affected by
        undefined C behavior in this implementation.

    SEE ALSO
        nextafterf(), nexttoward()

*****************************************************************************/
float
nexttowardf(float x, long double y)
{
	union IEEEl2bits uy;
	volatile float t;
	int32_t hx,ix;

	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;		/* |x| */
	uy.e = y;

	if((ix>0x7f800000) ||
	   (uy.bits.exp == LDBL_INFNAN_EXP &&
	    ((uy.bits.manh&~LDBL_NBIT)|uy.bits.manl) != 0))
	   return x+y;	/* x or y is nan */
	if(x==y) return (float)y;		/* x=y, return y */
	if(ix==0) {				/* x == 0 */
	    SET_FLOAT_WORD(x,(uy.bits.sign<<31)|1);/* return +-minsubnormal */
	    t = x*x;
	    if(t==x) return t; else return x;	/* raise underflow flag */
	}
	if(hx>=0 ^ x < y)			/* x -= ulp */
	    hx -= 1;
	else					/* x += ulp */
	    hx += 1;
	ix = hx&0x7f800000;
	if(ix>=0x7f800000) return x+x;	/* overflow  */
	if(ix<0x00800000) {		/* underflow */
	    t = x*x;
	    if(t!=x) {		/* raise underflow flag */
	        SET_FLOAT_WORD(x,hx);
		return x;
	    }
	}
	SET_FLOAT_WORD(x,hx);
	return x;
}

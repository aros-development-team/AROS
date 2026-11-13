/* @(#)s_nextafter.c 5.1 93/09/24 */
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_nextafterl.c,v 1.2 2008/02/22 02:30:36 das Exp $";
#endif

/* IEEE functions
 *	nextafter(x,y)
 *	return the next machine floating-point number of x in the
 *	direction toward y.
 *   Special cases:
 */

#include <aros/system.h>
#include <float.h>

#include "fpmath.h"
#include "math.h"
#include "math_private.h"

#if LDBL_MAX_EXP != 0x4000
#error "Unsupported long double format"
#endif

/****************************************************************************
    NAME
        long double nextafterl(

    SYNOPSIS
        long double x, long double y)

    FUNCTION
        Return the adjacent long double value in the direction of y.

    INPUTS
        x - Starting value.
        y - Value that determines the direction.

    RESULT
        If x and y compare equal, return y, including its zero sign.
        A zero starting value and nonzero direction select the smallest
        subnormal with the sign of y. If either argument is NaN, the
        result is NaN.

        An inward step from infinity returns the largest finite value
        with the same sign. See BUGS for outward steps at the finite limit.

    NOTES
        Include <math.h>. This function has the same behavior as
        nexttowardl().

        Subnormal results can raise FE_UNDERFLOW and FE_INEXACT. Outward
        steps at the finite limit can raise FE_OVERFLOW and FE_INEXACT.
        Enabled traps may prevent return. This function does not set errno.

    BUGS
        An outward step from the largest positive or negative finite
        value can return that same finite value under directed rounding,
        instead of the corresponding infinity.

        Where long double has greater precision than double, a step from
        the smallest subnormal to zero does not raise FE_UNDERFLOW or
        FE_INEXACT. Existing exception flags are not cleared.

    SEE ALSO
        nexttowardl(), nextafter(), nexttoward()

*****************************************************************************/
long double
nextafterl(long double x, long double y)
{
	volatile long double t;
	union IEEEl2bits ux, uy;

	ux.e = x;
	uy.e = y;

	if ((ux.bits.exp == 0x7fff &&
	     ((ux.bits.manh&~LDBL_NBIT)|ux.bits.manl) != 0) ||
	    (uy.bits.exp == 0x7fff &&
	     ((uy.bits.manh&~LDBL_NBIT)|uy.bits.manl) != 0))
	   return x+y;	/* x or y is nan */
	if(x==y) return y;		/* x=y, return y */
	if(x==0.0) {
	    ux.bits.manh = 0;			/* return +-minsubnormal */
	    ux.bits.manl = 1;
	    ux.bits.sign = uy.bits.sign;
	    t = ux.e*ux.e;
	    if(t==ux.e) return t; else return ux.e; /* raise underflow flag */
	}
	if(x>0.0 ^ x<y) {			/* x -= ulp */
	    if(ux.bits.manl==0) {
		if ((ux.bits.manh&~LDBL_NBIT)==0)
		    ux.bits.exp -= 1;
		ux.bits.manh = (ux.bits.manh - 1) | (ux.bits.manh & LDBL_NBIT);
	    }
	    ux.bits.manl -= 1;
	} else {				/* x += ulp */
	    ux.bits.manl += 1;
	    if(ux.bits.manl==0) {
		ux.bits.manh = (ux.bits.manh + 1) | (ux.bits.manh & LDBL_NBIT);
		if ((ux.bits.manh&~LDBL_NBIT)==0)
		    ux.bits.exp += 1;
	    }
	}
	if(ux.bits.exp==0x7fff) return x+x;	/* overflow  */
	if(ux.bits.exp==0) {			/* underflow */
	    mask_nbit_l(ux);
	    t = ux.e * ux.e;
	    if(t!=ux.e)			/* raise underflow flag */
		return ux.e;
	}
	return ux.e;
}

/****************************************************************************
    NAME
        long double nexttowardl(

    SYNOPSIS
        long double x, long double y)

    FUNCTION
        Return the adjacent long double value in the direction of y.

    INPUTS
        x - Starting value.
        y - Value that determines the direction.

    RESULT
        If x and y compare equal, return y, including its zero sign.
        A zero starting value and nonzero direction select the smallest
        subnormal with the sign of y. If either argument is NaN, the
        result is NaN.

        An inward step from infinity returns the largest finite value
        with the same sign. See BUGS for outward steps at the finite limit.

    NOTES
        Include <math.h>. This function has the same behavior as
        nextafterl().

        Subnormal results can raise FE_UNDERFLOW and FE_INEXACT. Outward
        steps at the finite limit can raise FE_OVERFLOW and FE_INEXACT.
        Enabled traps may prevent return. This function does not set errno.

    BUGS
        An outward step from the largest positive or negative finite
        value can return that same finite value under directed rounding,
        instead of the corresponding infinity.

        Where long double has greater precision than double, a step from
        the smallest subnormal to zero does not raise FE_UNDERFLOW or
        FE_INEXACT. Existing exception flags are not cleared.

    SEE ALSO
        nextafterl(), nextafter(), nexttoward()

*****************************************************************************/
AROS_MAKE_ALIAS(nextafterl, nexttowardl);

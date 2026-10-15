/* s_scalbnf.c -- float version of s_scalbn.c.
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_scalbnf.c,v 1.8 2005/03/07 04:52:43 das Exp $";
#endif

#include <aros/system.h>

#include "math.h"
#include "math_private.h"

static const float
two25   =  3.355443200e+07,	/* 0x4c000000 */
twom25  =  2.9802322388e-08,	/* 0x33000000 */
huge   = 1.0e+30,
tiny   = 1.0e-30;

/****************************************************************************
    NAME
        float scalbnf(

    SYNOPSIS
        float x, int n)

    FUNCTION
        Scale x by an integral power of two.

    INPUTS
        x - Value to scale.
        n - Base-two exponent.

    RESULT
        The scaled float value. Inexact results are rounded according
        to the current floating-point rounding direction. Signed zero
        and infinity retain their signs; NaN produces NaN.

    NOTES
        Include <math.h>. Overflow and underflow may raise floating-point
        exceptions. This implementation does not set errno.
        ldexpf() is an alias of scalbnf().

    BUGS
        The implementation adds n to a signed exponent without checking
        for overflow. For x = 1 and n = INT_MAX, this causes undefined
        signed integer arithmetic before the range check.

    SEE ALSO
        ldexpf(), scalblnf(), frexpf()

*****************************************************************************/
float
scalbnf (float x, int n)
{
	int32_t k,ix;
	GET_FLOAT_WORD(ix,x);
        k = (ix&0x7f800000)>>23;		/* extract exponent */
        if (k==0) {				/* 0 or subnormal x */
            if ((ix&0x7fffffff)==0) return x; /* +-0 */
	    x *= two25;
	    GET_FLOAT_WORD(ix,x);
	    k = ((ix&0x7f800000)>>23) - 25;
            if (n< -50000) return tiny*x; 	/*underflow*/
	    }
        if (k==0xff) return x+x;		/* NaN or Inf */
        k = k+n;
        if (k >  0xfe) return huge*copysignf(huge,x); /* overflow  */
        if (k > 0) 				/* normal result */
	    {SET_FLOAT_WORD(x,(ix&0x807fffff)|(k<<23)); return x;}
        if (k <= -25) {
            if (n > 50000) 	/* in case integer overflow in n+k */
		return huge*copysignf(huge,x);	/*overflow*/
	    else
		return tiny*copysignf(tiny,x);	/*underflow*/
	}
        k += 25;				/* subnormal result */
	SET_FLOAT_WORD(x,(ix&0x807fffff)|(k<<23));
        return x*twom25;
}

/****************************************************************************
    NAME
        float ldexpf(

    SYNOPSIS
        float x, int n)

    FUNCTION
        Multiply x by 2 raised to n.

    INPUTS
        x - Value to scale.
        n - Base-two exponent.

    RESULT
        The scaled float value. Inexact results are rounded according
        to the current floating-point rounding direction. Signed zero
        and infinity retain their signs; NaN produces NaN.

    NOTES
        Include <math.h>. ldexpf() is a symbol alias of scalbnf(),
        with identical numerical and exception behavior. This
        implementation does not set errno. Overflow and underflow may
        raise floating-point exceptions.

    BUGS
        The shared implementation adds n to a signed exponent without
        checking for overflow. For x = 1 and n = INT_MAX, this causes
        undefined signed integer arithmetic before the range check.

    SEE ALSO
        scalbnf(), scalblnf(), frexpf()

*****************************************************************************/
AROS_MAKE_ALIAS(scalbnf, ldexpf);

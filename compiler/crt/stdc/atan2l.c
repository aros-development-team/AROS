/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function cbrtl
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if	LDBL_MANT_DIG != DBL_MANT_DIG
/*****************************************************************************

    NAME */
#include <math.h>

        long double atan2l(

/*  SYNOPSIS */
        long double y, long double x)

/*  FUNCTION
        Computes the arc tangent of y/x considering the signs of both arguments
        to determine the correct quadrant of the result.

    INPUTS
        y - the vertical component.
        x - the horizontal component.

    RESULT
        Returns angle in radians in the range [-p, p].
        Returns NaN and sets errno to EDOM if x and y are both 0.

    NOTES
        This is the long double precision variant of atan2().

    EXAMPLE
        long double angle = atan2l(1.0L, -1.0L);  // returns ~2.356L

    BUGS
        None known.

    SEE ALSO
        atan2(), atan(), atan2f()

    INTERNALS
        Uses __ieee754_atan2l() with domain checks.

******************************************************************************/
{
    if (isnan(x) || isnan(y)) return x + y;
    if (x == 0.0L && y == 0.0L) {
        feraiseexcept(FE_INVALID);
        errno = EDOM;
        return NAN;
    }
    return __ieee754_atan2l(y, x);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

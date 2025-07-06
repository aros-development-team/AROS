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

        long double powl(

/*  SYNOPSIS */
        long double x, long double y)

/*  FUNCTION
        Computes x raised to the power y.

    INPUTS
        x - base value.
        y - exponent.

    RESULT
        Returns x^y.

    NOTES
        Domain error if x < 0 and y is not an integer.

    EXAMPLE
        long double r = powl(2.0L, 3.0L);  // returns 8.0L

    BUGS
        None known.

    SEE ALSO
        expl(), logl()

    INTERNALS
        Uses __ieee754_powl() with domain and pole checks.

******************************************************************************/
{
    if (isnan(x) || isnan(y)) return x + y;

    // Domain errors for real-valued pow
    if (x == 0.0L && y < 0.0L) {
        errno = EDOM;
        feraiseexcept(FE_DIVBYZERO);
        return HUGE_VALL;
    }
    if (x < 0.0L && floorl(y) != y) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }

    return __ieee754_powl(x, y);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

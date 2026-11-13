/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

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
    long double r;

    if (isnan(x) || isnan(y)) return x + y;

    // Domain error: negative base with non-integer exponent
    if (x < 0.0L && floorl(y) != y) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }

    r = __ieee754_powl(x, y);

    if (x == 0.0L && y < 0.0L) {
        /* Pole error: zero raised to a negative power (Annex F.10.4.4). */
        errno = ERANGE;
        feraiseexcept(FE_DIVBYZERO);
    } else if (!isfinite(r) && isfinite(x) && isfinite(y)) {
        /* Range error: overflow (a non-finite result from finite inputs). */
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW);
    }

    return r;
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

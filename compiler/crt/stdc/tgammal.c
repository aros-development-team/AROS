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

        long double tgammal(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the gamma function G(x).

    INPUTS
        x - input value.

    RESULT
        Returns G(x), or 8 for poles.

    NOTES
        Undefined for negative integers.

    EXAMPLE
        long double r = tgammal(5.0L);  // returns 24.0L

    BUGS
        None known.

    SEE ALSO
        lgammal(), gamma()

    INTERNALS
        Uses __ieee754_tgammal() with pole checks.

******************************************************************************/
{
    long double r;

    if (isnan(x)) return x;

    r = __ieee754_tgammal(x);
    if (isnan(r)) {
        /* Domain error: a negative integer argument (Annex F.10.5.4). */
        errno = EDOM;
        feraiseexcept(FE_INVALID);
    } else if (isinf(r) && isfinite(x)) {
        if (x == 0.0L) {
            /* Pole error at zero. */
            errno = ERANGE;
            feraiseexcept(FE_DIVBYZERO);
        } else {
            /* Range error: overflow for large x. */
            errno = ERANGE;
            feraiseexcept(FE_OVERFLOW);
        }
    }
    return r;
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

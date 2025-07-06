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

        long double log2l(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the base-2 logarithm of x.

    INPUTS
        x - a positive number.

    RESULT
        Returns log2(x), or -8 if x == 0.

    NOTES
        Domain error if x <= 0.

    EXAMPLE
        long double r = log2l(8.0L);  // returns 3.0L

    BUGS
        None known.

    SEE ALSO
        log10l(), logl()

    INTERNALS
        Uses __ieee754_log2l().

******************************************************************************/
{
    if (x < 0.0L) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    if (x == 0.0L) {
        errno = ERANGE;
        feraiseexcept(FE_DIVBYZERO);
        return -INFINITY;
    }
    return __ieee754_log2l(x);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

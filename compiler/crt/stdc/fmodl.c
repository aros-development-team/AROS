/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function fmodl
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if LDBL_MANT_DIG != DBL_MANT_DIG
/*****************************************************************************

    NAME */
#include <math.h>

        long double fmodl(

/*  SYNOPSIS */
        long double x,
        long double y)

/*  FUNCTION
        Computes floating-point remainder for long double precision.

    INPUTS
        x - numerator.
        y - denominator (non-zero).

    RESULT
        Floating-point remainder.

    NOTES
        Same rules as fmod().

    EXAMPLE
        long double r = fmodl(5.3L, 2.0L);  // r = 1.3L

    BUGS
        None known.

    SEE ALSO
        fmod(), fmodf()

    INTERNALS
        Calls __ieee754_fmodl(x, y).

******************************************************************************/
{
//    FORWARD_IF_NAN_OR_INF2(fmodl, x, y);

    // Handle special cases: y == 0 or x infinite ? domain error
    if (y == 0.0 || !isfinite(x)) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return __builtin_nan("");  // or (x - x) as NaN
    }

    double r = 0;
    //r = __ieee754_fmodl(x, y);

    // fmod should never overflow, so no ERANGE check here

    return r;
}
#endif

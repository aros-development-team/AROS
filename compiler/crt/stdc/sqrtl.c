/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function sqrtl
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if LDBL_MANT_DIG != DBL_MANT_DIG
/*****************************************************************************

    NAME */
#include <math.h>

        long double sqrtl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the square root of x with extended precision.

    INPUTS
        x - input value (must be non-negative).

    RESULT
        Returns vx as a long double.
        Returns NaN and sets errno to EDOM if x < 0.

    NOTES
        Provides higher precision than sqrt().

    EXAMPLE
        long double val = sqrtl(4.0L);  // returns 2.0L

    BUGS
        None known.

    SEE ALSO
        sqrt(), cbrtl()

    INTERNALS
        Forwards to __ieee754_sqrtl() with domain and error handling.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(sqrtl, x);
    if (x < 0.0) {
        STDC_SETMATHERRNO(EDOM)
        STDC_RAISEMATHEXCPT(FE_INVALID)
        return NAN;
    }
    return __ieee754_sqrtl(x);
}
#endif

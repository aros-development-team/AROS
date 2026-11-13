/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function cos
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if LDBL_MANT_DIG != DBL_MANT_DIG
/*****************************************************************************

    NAME */
#include <math.h>

        long double cosl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the cosine of x (x in radians).

    INPUTS
        x - angle in radians.

    RESULT
        Returns the cosine of x.

    NOTES
        Result is in the range [-1, 1].

    EXAMPLE
        double c = cos(0.0);  // returns 1.0

    BUGS
        None known.

    SEE ALSO
        sin(), tan(), cosf(), cosl()

    INTERNALS
        Forwards to __ieee754_cos().

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(cosl, x);
    return __ieee754_cosl(x);
}
#endif

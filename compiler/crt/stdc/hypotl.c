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

        long double hypotl(

/*  SYNOPSIS */
        long double x, long double y)

/*  FUNCTION
        Computes sqrt(x² + y²) without undue overflow or underflow.

    INPUTS
        x - first value.
        y - second value.

    RESULT
        Returns the Euclidean norm.

    NOTES
        Accurate even if x or y is large/small.

    EXAMPLE
        long double r = hypotl(3.0L, 4.0L);  // returns 5.0L

    BUGS
        None known.

    SEE ALSO
        sqrtl(), fabs()

    INTERNALS
        Uses __ieee754_hypotl().

******************************************************************************/
{
    if (isinf(x) || isinf(y)) return INFINITY;
    if (isnan(x) || isnan(y)) return x + y;
    return __ieee754_hypotl(x, y);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

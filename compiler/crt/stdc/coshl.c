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

        long double coshl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the hyperbolic cosine of x.

    INPUTS
        x - input angle in radians.

    RESULT
        Returns the hyperbolic cosine of x.

    NOTES
        Returns positive values = 1.

    EXAMPLE
        long double r = coshl(0.0L);  // returns 1.0L

    BUGS
        None known.

    SEE ALSO
        sinhl(), tanhl(), cosl()

    INTERNALS
        Uses __ieee754_coshl().

******************************************************************************/
{
    if (isnan(x)) return x;
    return __ieee754_coshl(x);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

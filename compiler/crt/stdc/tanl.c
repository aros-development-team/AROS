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

        long double tanl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the tangent of x.

    INPUTS
        x - angle in radians.

    RESULT
        Returns tan(x), or may overflow near odd multiples of p/2.

    NOTES
        Long double variant of tan().

    EXAMPLE
        long double r = tanl(0.0L);  // returns 0.0L

    BUGS
        None known.

    SEE ALSO
        sinl(), cosl()

    INTERNALS
        Uses __ieee754_tanl().

******************************************************************************/
{
    if (isnan(x)) return x;
    return __ieee754_tanl(x);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

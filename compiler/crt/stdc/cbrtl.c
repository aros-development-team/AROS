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

        long double cbrtl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the cube root of a long double value.

    INPUTS
        x - input value.

    RESULT
        Returns the cube root of x.

    NOTES
        Works with negative values as well.

    EXAMPLE
        long double r = cbrtl(-8.0L);  // returns -2.0L

    BUGS
        None known.

    SEE ALSO
        sqrtl(), powl()

    INTERNALS
        Calls __ieee754_cbrtl().

******************************************************************************/
{
    if (isnan(x)) return x;
    return __ieee754_cbrtl(x);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

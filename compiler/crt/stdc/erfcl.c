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

        long double erfcl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the complementary error function of x.

    INPUTS
        x - a long double value.

    RESULT
        Returns 1 - erf(x).

    NOTES
        Useful for calculating tail probabilities.

    EXAMPLE
        long double r = erfcl(0.0L);  // returns 1.0L

    BUGS
        None known.

    SEE ALSO
        erfl(), erf(), erfclf()

    INTERNALS
        Calls __ieee754_erfcl().

******************************************************************************/
{
    if (isnan(x)) return x;
    return __ieee754_erfcl(x);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

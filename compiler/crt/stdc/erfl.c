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

        long double erfl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the error function of x.

    INPUTS
        x - a long double value.

    RESULT
        Returns erf(x).

    NOTES
        Related to Gaussian integrals.

    EXAMPLE
        long double r = erfl(0.0L);  // returns 0.0L

    BUGS
        None known.

    SEE ALSO
        erf(), erfcl()

    INTERNALS
        Calls __ieee754_erfl().

******************************************************************************/
{
    if (isnan(x)) return x;
    return __ieee754_erfl(x);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

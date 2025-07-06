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

        long double lgammal(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the natural logarithm of the absolute value of the gamma function.

    INPUTS
        x - input value.

    RESULT
        Returns ln|G(x)|.

    NOTES
        Useful for large factorials: lgammal(n+1) ˜ ln(n!)

    EXAMPLE
        long double r = lgammal(5.0L);  // ˜ ln(24)

    BUGS
        None known.

    SEE ALSO
        tgammal(), gamma()

    INTERNALS
        Uses __ieee754_lgammal().

******************************************************************************/
{
    if (isnan(x)) return x;
    return __ieee754_lgammal(x);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

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

        long double modfl(

/*  SYNOPSIS */
        long double x, long double *iptr)

/*  FUNCTION
        Splits x into integer and fractional parts.

    INPUTS
        x - input value.
        iptr - pointer to store the integer part.

    RESULT
        Returns the fractional part (x - *iptr).

    NOTES
        Does not raise domain errors.

    EXAMPLE
        long double i;
        long double f = modfl(2.718L, &i);  // i = 2.0L, f ˜ 0.718L

    BUGS
        None known.

    SEE ALSO
        floorl(), ceill()

    INTERNALS
        Uses __ieee754_modfl().

******************************************************************************/
{
    return __ieee754_modfl(x, iptr);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

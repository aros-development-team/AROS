/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function atan
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if LDBL_MANT_DIG != DBL_MANT_DIG
/*****************************************************************************

    NAME */
#include <math.h>

        long double atanl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the arc tangent of x in radians.

    INPUTS
        x - a long double value.

    RESULT
        Returns value in range [-p/2, p/2].

    NOTES
        Long double precision version of atan().

    EXAMPLE
        long double r = atanl(1.0L);  // returns ~0.785L

    BUGS
        None known.

    SEE ALSO
        atan(), atan2l()

    INTERNALS
        Calls __ieee754_atanl() directly.

******************************************************************************/
 {
    if (isnan(x)) return x;
    return __ieee754_atanl(x);
}
#endif

/*****************************************************************************

    NAME */
#include <math.h>

        float sqrtf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes square root of x.

    INPUTS
        x - input float (= 0)

    RESULT
        Square root of x.

    NOTES
        Returns NaN if x < 0.

    EXAMPLE
        float r = sqrtf(4.0f); // 2.0f

    BUGS
        None known.

    SEE ALSO
        sqrt(), sqrtl()

    INTERNALS
        Uses __ieee754_sqrtf() with Newton iteration and domain checks.

******************************************************************************/
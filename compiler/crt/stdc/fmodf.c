/*****************************************************************************

    NAME */
#include <math.h>

        float fmodf(

/*  SYNOPSIS */
        float x,
        float y)

/*  FUNCTION
        Computes remainder of x / y.

    INPUTS
        x - numerator.
        y - denominator.

    RESULT
        Floating-point remainder.

    NOTES
        Result sign matches x.
        y=0 returns NaN.

    EXAMPLE
        float v = fmodf(5.3f, 2.0f); // 1.3

    BUGS
        None known.

    SEE ALSO
        remainderf()

    INTERNALS
        Uses __ieee754_fmodf() which subtracts multiples of y from x carefully.

******************************************************************************/
/*****************************************************************************

    NAME */
#include <math.h>

        float tanf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the tangent of x (in radians).

    INPUTS
        x - angle in radians.

    RESULT
        Tangent of x.

    NOTES
        tan(x) = sin(x) / cos(x), undefined at (p/2 + kp)

    EXAMPLE
        float r = tanf(0.785398f); // ˜ 1.0 (p/4 radians)

    BUGS
        Near p/2 and odd multiples, large errors or infinities may result.

    SEE ALSO
        sinf(), cosf()

    INTERNALS
        Delegates to __ieee754_tanf().
        Performs argument reduction with quadrant analysis.
        Returns appropriate IEEE exceptions for infinite or NaN input.

******************************************************************************/
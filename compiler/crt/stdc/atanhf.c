/*****************************************************************************

    NAME */
#include <math.h>

        float atanhf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the inverse hyperbolic tangent of x (float version).

    INPUTS
        x - value in range (-1, 1).

    RESULT
        atanhf(x)

    NOTES
        Returns NaN for |x| = 1.
        Very large values close to 1 may result in loss of precision.

    EXAMPLE
        float v = atanhf(0.5f); // ˜ 0.5493f

    BUGS
        None known.

    SEE ALSO
        tanhf(), asinhf(), acoshf()

    INTERNALS
        Implemented using __ieee754_atanhf().
        Based on formula: 0.5 * log((1 + x) / (1 - x)).
        Includes domain checks to avoid invalid input.

******************************************************************************/
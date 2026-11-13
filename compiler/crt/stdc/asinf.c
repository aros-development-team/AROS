/*****************************************************************************

    NAME */
#include <math.h>

        float asinf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the principal value of the arc sine of x.

    INPUTS
        x - a float value in the domain [-1.0f, 1.0f]

    RESULT
        The arc sine of x, in radians, in the range [-p/2, p/2].

    NOTES
        Returns NaN if input is outside [-1,1], with errno set.
        Precision may degrade near domain limits.

    EXAMPLE
        float y = asinf(0.5f);  // y ˜ 0.5235987756f (p/6)
        float z = asinf(-1.0f); // z = -p/2

    BUGS
        Minor precision issues near domain edges.

    SEE ALSO
        asin(), acosf(), atanf()

    INTERNALS
        Calls __ieee754_asinf(x).
        Domain checked; returns NaN and sets errno on out-of-range input.
        Uses rational polynomial approximations.

******************************************************************************/
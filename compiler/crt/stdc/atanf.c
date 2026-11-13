/*****************************************************************************

    NAME */
#include <math.h>

        float atanf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the principal value of the arc tangent of x.

    INPUTS
        x - a float value (any real number)

    RESULT
        The arc tangent of x, in radians, in the range [-p/2, p/2].

    NOTES
        Defined for all real floats.
        Returns NaN if input is NaN.

    EXAMPLE
        float y = atanf(0.0f);    // y = 0.0f
        float z = atanf(-1.0f);   // z = -p/4 ˜ -0.78539816339f

    BUGS
        None known.

    SEE ALSO
        atan(), atan2f(), tanf()

    INTERNALS
        Calls __ieee754_atanf(x).
        Uses argument reduction and polynomial approximations.
        Handles edge cases and special floating-point values.

******************************************************************************/
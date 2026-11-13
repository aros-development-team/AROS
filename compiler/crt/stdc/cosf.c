/*****************************************************************************

    NAME */
#include <math.h>

        float cosf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the cosine of x (x in radians).

    INPUTS
        x - a float value representing an angle in radians.

    RESULT
        The cosine of the angle x.

    NOTES
        Periodic with period 2p.
        Returns NaN if input is NaN.
        May lose precision for large magnitude inputs.

    EXAMPLE
        float y = cosf(0.0f);         // y = 1.0f
        float z = cosf(3.14159f / 2); // z ˜ 0.0f

    BUGS
        Precision degradation on large inputs.

    SEE ALSO
        cos(), sinf(), tanf()

    INTERNALS
        Calls __ieee754_cosf(x).
        Uses argument reduction and polynomial approximations.
        Handles special cases for inputs near multiples of p/2.

******************************************************************************/
/*****************************************************************************

    NAME */
#include <math.h>

        float atan2f(

/*  SYNOPSIS */
        float y,
        float x)

/*  FUNCTION
        Computes the arc tangent of y/x with correct quadrant determination.

    INPUTS
        y - numerator (float)
        x - denominator (float)

    RESULT
        The angle ? between the positive x-axis and the point (x,y), in radians,
        in the range [-p, p].

    NOTES
        Correctly handles special cases (x = 0, y = 0).
        Returns NaN if either argument is NaN.

    EXAMPLE
        float angle = atan2f(0.0f, -1.0f);  // p
        float angle2 = atan2f(1.0f, 0.0f);  // p/2

    BUGS
        None known.

    SEE ALSO
        atan(), atan2(), tanf()

    INTERNALS
        Calls __ieee754_atan2f(y, x).
        Uses sign analysis for quadrant determination.
        Uses polynomial approximations and argument reduction.

******************************************************************************/
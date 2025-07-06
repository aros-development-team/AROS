/*****************************************************************************

    NAME */
#include <math.h>

        float cbrtf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the cube root of x.

    INPUTS
        x - a float value (any real number)

    RESULT
        The cube root of x.

    NOTES
        Works for negative and positive values.
        Returns NaN if input is NaN.

    EXAMPLE
        float y = cbrtf(8.0f);    // y = 2.0f
        float z = cbrtf(-27.0f);  // z = -3.0f

    BUGS
        None known.

    SEE ALSO
        cbrt(), powf()

    INTERNALS
        Calls __ieee754_cbrtf(x).
        Uses polynomial approximation and Newton-Raphson iteration.
        Maintains sign correctness and precision.

******************************************************************************/
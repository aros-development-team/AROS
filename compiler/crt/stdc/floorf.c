/*****************************************************************************

    NAME */
#include <math.h>

        float floorf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Returns largest integer <= x.

    INPUTS
        x - value.

    RESULT
        Floor of x.

    NOTES
        Handles negatives and special values.

    EXAMPLE
        float v = floorf(-2.3f); // -3.0

    BUGS
        None known.

    SEE ALSO
        ceilf(), truncf()

    INTERNALS
        Uses __ieee754_floorf() with rounding techniques.

******************************************************************************/
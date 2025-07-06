/*****************************************************************************

    NAME */
#include <math.h>

        float ceilf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Returns smallest integer not less than x.

    INPUTS
        x - input float

    RESULT
        Ceiling of x.

    NOTES
        Rounds up fractional values.

    EXAMPLE
        float r = ceilf(2.3f);  // 3.0f
        float r2 = ceilf(-2.3f); // -2.0f

    BUGS
        None known.

    SEE ALSO
        floorf(), roundf(), truncf()

    INTERNALS
        Uses __ieee754_floorf() internally and adjusts for negative values.

******************************************************************************/
/*****************************************************************************

    NAME */
#include <math.h>

        float hypotf(

/*  SYNOPSIS */
        float x,
        float y)

/*  FUNCTION
        Returns sqrt(x² + y²) without overflow/underflow.

    INPUTS
        x, y - values.

    RESULT
        Euclidean distance.

    NOTES
        Safer than naive sqrt(x*x + y*y).

    EXAMPLE
        float v = hypotf(3.0f, 4.0f); // 5.0

    BUGS
        None known.

    SEE ALSO
        sqrtf()

    INTERNALS
        Calls __ieee754_hypotf() with scaling to avoid intermediate overflow.

******************************************************************************/
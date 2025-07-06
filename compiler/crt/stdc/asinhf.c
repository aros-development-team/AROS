/*****************************************************************************

    NAME */
#include <math.h>

        float asinhf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes inverse hyperbolic sine (float version).

    INPUTS
        x - real float.

    RESULT
        asinhf(x)

    NOTES
        Accurate for small and large x.

    EXAMPLE
        float v = asinhf(1.0f); // ˜ 0.881373f

    BUGS
        None known.

    SEE ALSO
        asinh(), sinhf()

    INTERNALS
        Implements __ieee754_asinhf() using log and sqrt for precision.

******************************************************************************/
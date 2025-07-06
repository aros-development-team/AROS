/*****************************************************************************

    NAME */
#include <math.h>

        float sinhf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the hyperbolic sine of x.

    INPUTS
        x - input float

    RESULT
        Returns sinh(x).

    NOTES
        Uses exponential functions internally.

    EXAMPLE
        float r = sinhf(1.0f); // ˜ 1.175201

    BUGS
        None known.

    SEE ALSO
        coshf(), tanhf(), sinh()

    INTERNALS
        Implements sinhf via __ieee754_expf() to calculate (exp(x) - exp(-x))/2,
        with overflow handling and linear approximation near zero.

******************************************************************************/
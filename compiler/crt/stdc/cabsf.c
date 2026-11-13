/*****************************************************************************

    NAME */
#include <complex.h>
#include <math.h>

        float cabsf(

/*  SYNOPSIS */
        float complex z)

/*  FUNCTION
        Computes magnitude of complex float.

    INPUTS
        z - complex number.

    RESULT
        sqrt(Re(z)^2 + Im(z)^2)

    NOTES
        Uses scaling to avoid overflow/underflow.

    EXAMPLE
        float complex z = 3.0f + 4.0f*I;
        float v = cabsf(z); // 5.0

    BUGS
        None known.

    SEE ALSO
        hypotf()

    INTERNALS
        Calls __ieee754_hypotf() on real and imaginary parts.

******************************************************************************/
/*****************************************************************************

    NAME */
#include <math.h>

        float coshf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the hyperbolic cosine of x.

    INPUTS
        x - a float value.

    RESULT
        The hyperbolic cosine of x, (e^x + e^(-x)) / 2.

    NOTES
        Returns values = 1.
        Returns NaN if input is NaN.
        May overflow to +Inf for large |x|.

    EXAMPLE
        float y = coshf(0.0f);    // y = 1.0f
        float z = coshf(1.0f);    // z ˜ 1.543081f

    BUGS
        Overflow for large magnitude inputs.

    SEE ALSO
        cosh(), sinhf(), tanhf()

    INTERNALS
        Calls __ieee754_coshf(x).
        Uses calls to expf for calculation.
        Uses scaling to reduce overflow risk when possible.

******************************************************************************/
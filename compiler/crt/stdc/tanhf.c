/*****************************************************************************

    NAME */
#include <math.h>

        float tanhf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes hyperbolic tangent of x.

    INPUTS
        x - input float

    RESULT
        Returns tanh(x).

    NOTES
        Approaches ±1 for large |x|.

    EXAMPLE
        float r = tanhf(1.0f); // ˜ 0.761594

    BUGS
        None known.

    SEE ALSO
        sinhf(), coshf()

    INTERNALS
        Computes tanhf via __ieee754_expf() to evaluate (exp(2x)-1)/(exp(2x)+1),
        with special handling of large inputs and zero.

******************************************************************************/

/*****************************************************************************

    NAME */
#include <math.h>

        float expm1f(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes e^x - 1 with improved precision for small x.

    INPUTS
        x - a float value.

    RESULT
        The value of e^x - 1.

    NOTES
        Especially accurate for values of x near zero.
        Returns NaN if input is NaN.

    EXAMPLE
        float y = expm1f(0.0f);    // y = 0.0f
        float z = expm1f(1e-7f);   // z ˜ 1e-7f

    BUGS
        None known.

    SEE ALSO
        expf(), expm1()

    INTERNALS
        Calls __ieee754_expm1f(x).
        Uses polynomial approximations for small x.
        Falls back to expf(x) - 1 for large |x|.

******************************************************************************/
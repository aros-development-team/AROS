/*****************************************************************************

    NAME */
#include <math.h>

        float acoshf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the inverse hyperbolic cosine of x.

    INPUTS
        x - a float value where x >= 1.0f

    RESULT
        The inverse hyperbolic cosine of x.

    NOTES
        Defined only for x = 1.
        Returns NaN and sets errno if input is outside domain.
        Uses approximations for numerical stability.

    EXAMPLE
        float y = acoshf(1.0f);  // y = 0.0f
        float z = acoshf(3.0f);  // z ˜ 1.7627471740f

    BUGS
        Minor precision loss near x=1.

    SEE ALSO
        acosh(), asinhf(), coshf()

    INTERNALS
        Calls __ieee754_acoshf(x).
        Checks domain and returns NaN with errno EDOM if x < 1.
        Uses logarithmic formula with range reduction.

******************************************************************************/
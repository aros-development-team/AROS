/*****************************************************************************

    NAME */
#include <math.h>

        float acosf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the principal value of the arc cosine of x.

    INPUTS
        x - a float value in the domain [-1.0f, 1.0f]

    RESULT
        The arc cosine of x, in radians, in the range [0, p].

    NOTES
        acosf(x) returns NaN if x is outside [-1,1].

    EXAMPLE
        float y = acosf(0.5f);  // y ˜ 1.0471975512f (p/3)
        float z = acosf(-1.0f); // z = p

    BUGS
        Precision loss near domain boundaries possible.

    SEE ALSO
        acos(), asinf(), atanf()

    INTERNALS
        Calls __ieee754_acosf(x).
        Domain checked; returns NaN with errno EDOM if input out of [-1,1].
        Uses range reduction and polynomial approximation.

******************************************************************************/
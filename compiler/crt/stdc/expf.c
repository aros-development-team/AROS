/*****************************************************************************

    NAME */
#include <math.h>

        float expf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the exponential function of x, e raised to the power x.

    INPUTS
        x - a float value.

    RESULT
        The value of e^x.

    NOTES
        Returns +Inf on overflow, 0 on underflow.
        Returns NaN if input is NaN.

    EXAMPLE
        float y = expf(0.0f);    // y = 1.0f
        float z = expf(1.0f);    // z ˜ 2.7182817f

    BUGS
        Precision loss for very large magnitude inputs.

    SEE ALSO
        exp(), expm1f()

    INTERNALS
        Calls __ieee754_expf(x).
        Uses argument reduction and polynomial approximation.
        Handles edge cases for overflow/underflow carefully.

******************************************************************************/
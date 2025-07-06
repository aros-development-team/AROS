/*****************************************************************************

    NAME */
#include <math.h>

        float powf(

/*  SYNOPSIS */
        float x,
        float y)

/*  FUNCTION
        Computes x raised to the power y.

    INPUTS
        x - base value.
        y - exponent.

    RESULT
        x raised to the power y as a float.

    NOTES
        If x is negative and y is not an integer, the result is a domain error.

    EXAMPLE
        float r = powf(2.0f, 3.0f); // 8.0f

    BUGS
        May return NaN for invalid domains, like powf(-2.0f, 0.5f)

    SEE ALSO
        pow(), exp(), log()

    INTERNALS
        Delegates to __ieee754_powf().
        Performs extensive domain and range checks, including NaN/Inf propagation and odd/even integer exponent logic.

******************************************************************************/

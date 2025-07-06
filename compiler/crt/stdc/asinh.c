/*****************************************************************************

    NAME */
#include <math.h>

        double asinh(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the inverse hyperbolic sine of x.

    INPUTS
        x - a double value (any real number)

    RESULT
        The inverse hyperbolic sine of x.

    NOTES
        asinh is defined for all real numbers.
        For large |x|, asinh(x) ˜ ln(2x), ensuring precision.
        Returns NaN if input is NaN.

    EXAMPLE
        double y = asinh(0.0);   // y = 0.0
        double z = asinh(1.0);   // z ˜ 0.8813735870

    BUGS
        None known.

    SEE ALSO
        asinhf(), sinh(), log()

    INTERNALS
        Calls __ieee754_asinh(x).
        Uses logarithmic identity: asinh(x) = ln(x + sqrt(x*x +1)).
        Handles special cases and overflow internally.

******************************************************************************/
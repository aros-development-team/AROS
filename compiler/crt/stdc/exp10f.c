/*****************************************************************************

    NAME */
#include <math.h>

        float exp10f(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes 10^x using exp(x * ln(10)).

    INPUTS
        x - exponent.

    RESULT
        10^x

    NOTES
        Domain: all real numbers.

    EXAMPLE
        float v = exp10f(2.0f); // 100.0

    BUGS
        None known.

    SEE ALSO
        expf(), powf()

    INTERNALS
        Uses __ieee754_expf() internally with argument scaled by ln(10).

******************************************************************************/

/*****************************************************************************

    NAME */
#include <math.h>

        double frexp(

/*  SYNOPSIS */
        double x,
        int *exp)

/*  FUNCTION
        Breaks the floating-point number x into its normalized fraction
        and an integral power of two. The fraction is returned, and the
        power of two is stored in *exp.

    INPUTS
        x - input floating point number.
        exp - pointer to integer where exponent will be stored.

    RESULT
        The normalized fraction part of x in the range [0.5, 1.0) or zero.

    NOTES
        Handles zero, infinity, NaN properly.
        Domain: all real numbers.

    EXAMPLE
        int e;
        double frac = frexp(8.0, &e); // frac=0.5, e=4 (8.0 = 0.5 * 2^4)

    BUGS
        None known.

    SEE ALSO
        ldexp(), modf()

    INTERNALS
        Uses __ieee754_frexpl() or equivalent for extracting exponent and fraction.

******************************************************************************/
/*****************************************************************************

    NAME */
#include <math.h>

        double gamma(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the gamma function G(x), which extends the factorial function
        to real and complex numbers.

    INPUTS
        x - input value.

    RESULT
        The value of the gamma function at x.

    NOTES
        Domain excludes non-positive integers (poles).
        Returns infinity or NaN on poles.
        Uses Lanczos approximation or similar.

    EXAMPLE
        double val = gamma(5.0); // val == 24.0 (factorial 4!)

    BUGS
        Precision varies for large arguments.
        Overflow on large positive values.

    SEE ALSO
        tgamma(), lgamma()

    INTERNALS
        Calls __ieee754_gamma() for core computation, includes domain checks.

******************************************************************************/

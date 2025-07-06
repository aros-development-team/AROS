/*****************************************************************************

    NAME */
#include <math.h>

        double expm1(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes e^x - 1 with higher precision than using exp(x) - 1 directly,
        especially for small values of x.

    INPUTS
        x - a double value.

    RESULT
        The value of e^x - 1.

    NOTES
        Provides better accuracy for small x where exp(x) is close to 1.
        Returns NaN if input is NaN.
        Returns -1 if x is large negative (underflow).

    EXAMPLE
        double y = expm1(0.0);     // y = 0.0
        double z = expm1(1e-10);   // z ˜ 1e-10

    BUGS
        None known.

    SEE ALSO
        exp(), expm1f()

    INTERNALS
        Calls __ieee754_expm1(x).
        Uses series expansions or rational approximations for small x.
        Uses exp(x) - 1 for large |x| values.

******************************************************************************/
/*****************************************************************************

    NAME */
#include <math.h>

        float erfclf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the complementary error function (long double variant).

    INPUTS
        x - input value.

    RESULT
        The complementary error function value.

    NOTES
        Similar to erfcf(), but for long double precision.

    EXAMPLE
        float val = erfclf(0.0f); // val == 1.0f

    BUGS
        None known.

    SEE ALSO
        erfcf(), erf()

    INTERNALS
        Uses __ieee754_erfcl() internally.

******************************************************************************/
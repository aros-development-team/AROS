/*****************************************************************************

    NAME */
#include <math.h>

        double ldexp(

/*  SYNOPSIS */
        double x,
        int exp)

/*  FUNCTION
        Computes x * 2^exp.

    INPUTS
        x - base double
        exp - exponent integer

    RESULT
        x scaled by 2 to the exp.

    NOTES
        Used for binary exponent scaling.

    EXAMPLE
        double r = ldexp(1.5, 3); // 12.0

    BUGS
        None known.

    SEE ALSO
        scalbn(), frexp()

    INTERNALS
        Adjusts exponent bits directly using __ieee754_scalbn().

******************************************************************************/
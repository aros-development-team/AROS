/*****************************************************************************

    NAME */
#include <math.h>

        long double floorl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Returns the largest integer value less than or equal to x as a long double.

    INPUTS
        x - input long double number.

    RESULT
        The floor of x as a long double.

    NOTES
        If x is integral, returns x.
        Domain: all real numbers.

    EXAMPLE
        long double val = floorl(2.7L); // val == 2.0L

    BUGS
        None known.

    SEE ALSO
        ceill(), truncl()

    INTERNALS
        Uses __ieee754_floorl(x) internally.

******************************************************************************/
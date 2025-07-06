/*****************************************************************************

    NAME */
#include <math.h>

        double floor(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Returns the largest integer value less than or equal to x.

    INPUTS
        x - input floating point number.

    RESULT
        The floor of x as a double.

    NOTES
        If x is integral, returns x.
        Domain: all real numbers.

    EXAMPLE
        double val = floor(2.7); // val == 2.0

    BUGS
        None known.

    SEE ALSO
        ceil(), trunc()

    INTERNALS
        Uses __ieee754_floor(x) for calculation.

******************************************************************************/
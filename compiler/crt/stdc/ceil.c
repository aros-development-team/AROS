/*****************************************************************************

    NAME */
#include <math.h>

        double ceil(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Returns the smallest integer value greater than or equal to x.

    INPUTS
        x - input floating point number.

    RESULT
        The ceiling of x as a double.

    NOTES
        If x is already integral, returns x unchanged.
        Domain: all real numbers.

    EXAMPLE
        double val = ceil(2.3); // val == 3.0

    BUGS
        None known.

    SEE ALSO
        floor(), trunc()

    INTERNALS
        Calls __ieee754_ceil(x) to perform the computation.

******************************************************************************/
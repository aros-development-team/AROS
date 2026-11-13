/*****************************************************************************

    NAME */
#include <math.h>

        long double ceill(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Returns the smallest integer value greater than or equal to x as long double.

    INPUTS
        x - input long double number.

    RESULT
        The ceiling of x as a long double.

    NOTES
        If x is integral, returns x.
        Domain: all real numbers.

    EXAMPLE
        long double val = ceill(2.3L); // val == 3.0L

    BUGS
        None known.

    SEE ALSO
        floorl(), truncl()

    INTERNALS
        Calls __ieee754_ceill(x) internally.

******************************************************************************/
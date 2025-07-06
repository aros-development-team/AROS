/*****************************************************************************

    NAME */
#include <math.h>

        long double fabsl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Returns absolute value of x.

    INPUTS
        x - input long double

    RESULT
        Absolute value (non-negative) of x.

    NOTES
        Works for all real numbers.

    EXAMPLE
        long double r = fabsl(-3.5L); // 3.5L

    BUGS
        None known.

    SEE ALSO
        fabs(), fabsf()

    INTERNALS
        Clears sign bit of floating-point value.

******************************************************************************/
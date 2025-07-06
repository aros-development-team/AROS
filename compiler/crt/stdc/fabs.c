/*****************************************************************************

    NAME */
#include <math.h>

        double fabs(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the absolute value of a double.

    INPUTS
        x - a double value.

    RESULT
        Absolute value of x.

    NOTES
        Handles special cases like negative zero and NaN.

    EXAMPLE
        double a = fabs(-5.0);  // 5.0
        double b = fabs(3.2);   // 3.2

    BUGS
        None known.

    SEE ALSO
        fabsf(), fabsl()

    INTERNALS
        Calls __ieee754_fabs(x).

******************************************************************************/
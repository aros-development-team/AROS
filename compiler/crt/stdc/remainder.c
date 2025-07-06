
/*****************************************************************************

    NAME */
#include <math.h>

        double remainder(

/*  SYNOPSIS */
        double x,
        double y)

/*  FUNCTION
        Returns the IEEE 754-style remainder of x with respect to y.

    INPUTS
        x - dividend.
        y - divisor.

    RESULT
        Remainder r such that r = x - n*y, where n is the integer nearest to x/y.

    NOTES
        If the result is exactly halfway between two integers, the even integer is used.

    EXAMPLE
        double r = remainder(5.3, 2.0); // r = 1.3

    BUGS
        Returns NaN if y is zero or x or y is NaN.

    SEE ALSO
        fmod(), modf()

    INTERNALS
        Uses __ieee754_remainder().
        Applies symmetric rounding to the nearest integer multiple of y.

******************************************************************************/

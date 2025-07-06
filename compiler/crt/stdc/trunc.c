/*****************************************************************************

    NAME */
#include <math.h>

        double trunc(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Truncates fractional part of x toward zero.

    INPUTS
        x - input double

    RESULT
        Integral part of x without fraction.

    NOTES
        Differs from floor() by rounding toward zero.

    EXAMPLE
        double r = trunc(3.9);  // 3.0
        double r2 = trunc(-3.9); // -3.0

    BUGS
        None known.

    SEE ALSO
        floor(), ceil(), round()

    INTERNALS
        Bitwise clears fraction using floating-point representation;
        uses __ieee754_floor() internally where applicable.

******************************************************************************/
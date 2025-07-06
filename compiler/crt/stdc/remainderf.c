
/*****************************************************************************

    NAME */
#include <math.h>

        float remainderf(

/*  SYNOPSIS */
        float x,
        float y)

/*  FUNCTION
        Computes IEEE remainder of x/y.

    INPUTS
        x - dividend
        y - divisor

    RESULT
        Remainder with magnitude less than y/2.

    NOTES
        Different from fmodf(); may be negative.

    EXAMPLE
        float r = remainderf(5.3f, 2.0f); // 1.3f

    BUGS
        Returns NaN if y == 0.

    SEE ALSO
        fmodf(), remainder()

    INTERNALS
        Calls __ieee754_remainderf() for computation,
        handling rounding and special cases as per IEEE 754.

******************************************************************************/
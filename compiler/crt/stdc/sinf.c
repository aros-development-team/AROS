/*****************************************************************************

    NAME */
#include <math.h>

        float sinf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the sine of x (x in radians).

    INPUTS
        x - input angle in radians.

    RESULT
        The sine of x.

    NOTES
        Domain: all real numbers.

    EXAMPLE
        float val = sinf(3.14159f / 2); // val approx 1.0f

    BUGS
        None known.

    SEE ALSO
        cosf(), tanf()

    INTERNALS
        Uses __ieee754_sinf().

******************************************************************************/

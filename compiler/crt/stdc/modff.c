/*****************************************************************************

    NAME */
#include <math.h>

        float modff(

/*  SYNOPSIS */
        float x,
        float *iptr)

/*  FUNCTION
        Splits the float x into integer and fractional parts.

    INPUTS
        x    - floating point number to split.
        iptr - pointer to float that will receive the integer part.

    RESULT
        Returns the fractional part of x.

    NOTES
        The result and *iptr have the same sign as x.

    EXAMPLE
        float i;
        float f = modff(3.14f, &i); // i = 3.0f, f = 0.14f

    BUGS
        None known.

    SEE ALSO
        modf(), frexp()

    INTERNALS
        Implements IEEE-compatible split using direct exponent/mantissa inspection.

******************************************************************************/
/*****************************************************************************

    NAME */
#include <math.h>

        float fabsf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Returns absolute value of x.

    INPUTS
        x - value.

    RESULT
        Absolute value.

    NOTES
        Clears sign bit.

    EXAMPLE
        float v = fabsf(-3.14f); // 3.14

    BUGS
        None known.

    SEE ALSO
        fabs()

    INTERNALS
        Uses __ieee754_fabsf() which clears sign bit efficiently.

******************************************************************************/
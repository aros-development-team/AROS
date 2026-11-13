/*****************************************************************************

    NAME */
#include <math.h>

        float truncf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Truncates fractional part of x toward zero.

    INPUTS
        x - input float

    RESULT
        Integral part with fraction discarded.

    NOTES
        Differs from floorf() by rounding toward zero.

    EXAMPLE
        float r = truncf(2.9f);  // 2.0f
        float r2 = truncf(-2.9f); // -2.0f

    BUGS
        None known.

    SEE ALSO
        floorf(), ceilf(), roundf()

    INTERNALS
        Bitwise clears fractional bits; no rounding.

******************************************************************************/
/*****************************************************************************

    NAME */
#include <math.h>

        float erfcf(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the complementary error function of x.

    INPUTS
        x - a float value.

    RESULT
        The complementary error function of x.

    NOTES
        Defined as 1 - erf(x).
        Returns NaN if input is NaN.

    EXAMPLE
        float y = erfcf(0.0f);    // y = 1.0f
        float z = erfcf(1.0f);    // z ˜ 0.157299f

    BUGS
        Reduced accuracy for large negative inputs.

    SEE ALSO
        erfc(), erff(), erfclf()

    INTERNALS
        Calls __ieee754_erfcf(x).
        Uses rational approximations and argument reduction.
        Manages floating-point underflow/overflow carefully.

******************************************************************************/
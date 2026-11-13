/*****************************************************************************

    NAME */
#include <math.h>

        float erff(

/*  SYNOPSIS */
        float x)

/*  FUNCTION
        Computes the error function erf(x).

    INPUTS
        x - input float

    RESULT
        Value of erf(x).

    NOTES
        Common in statistics and PDEs.

    EXAMPLE
        float r = erff(1.0f); // ˜ 0.8427008f

    BUGS
        None known.

    SEE ALSO
        erfcf(), erf()

    INTERNALS
        Uses polynomial approximations and range reductions,
        implemented in __erff().

******************************************************************************/
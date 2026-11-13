/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function atan2
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double atan2(

/*  SYNOPSIS */
        double y,
        double x)

/*  FUNCTION
        Computes the arc tangent of y/x using the signs of both arguments
        to determine the correct quadrant of the return value.

    INPUTS
        y - numerator (double)
        x - denominator (double)

    RESULT
        The angle ? between the positive x-axis and the point (x,y), in radians,
        in the range [-p, p].

    NOTES
        Handles cases where x is zero to avoid division by zero.
        Returns NaN if either argument is NaN.
        Returns ±p/2 if x = 0 and y ? 0.

    EXAMPLE
        double angle = atan2(1.0, 1.0); // ˜ p/4 (0.78539816339)
        double angle2 = atan2(-1.0, -1.0); // ˜ -3p/4 (-2.35619449019)

    BUGS
        None known.

    SEE ALSO
        atan(), atan2f(), tan()

    INTERNALS
        Calls __ieee754_atan2(y, x).
        Handles quadrant determination using signs of y and x.
        Uses argument reduction and polynomial approximations internally.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF2(atan2, y, x);
    return __ieee754_atan2(y, x);
}

#if (LDBL_MANT_DIG == DBL_MANT_DIG)
AROS_MAKE_ASM_SYM(typeof(atan2l), atan2l, AROS_CSYM_FROM_ASM_NAME(atan2l), AROS_CSYM_FROM_ASM_NAME(atan2));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(atan2l));
#endif

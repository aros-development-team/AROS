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
        double y, double x)

/*  FUNCTION
        Computes the arc tangent of `y/x`, considering the quadrant of the
        point (x, y), and returns the angle in radians.

    INPUTS
        y - the vertical component.
        x - the horizontal component.

    RESULT
        Returns angle in radians in the range [-p, p].
        If both x and y are zero, returns NaN and sets errno to EDOM.

    NOTES
        The function is useful for converting Cartesian coordinates to
        polar coordinates.

    EXAMPLE
        double angle = atan2(1.0, 1.0);  // returns ~0.785 (p/4)

    BUGS
        None known.

    SEE ALSO
        atan(), acos(), asin(), atan2f(), atan2l()

    INTERNALS
        Uses __ieee754_atan2() with checks for zero inputs and domain errors.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF2(atan2, y, x);
    return __ieee754_atan2(y, x);
}

#if (LDBL_MANT_DIG == DBL_MANT_DIG)
AROS_MAKE_ASM_SYM(typeof(atan2l), atan2l, AROS_CSYM_FROM_ASM_NAME(atan2l), AROS_CSYM_FROM_ASM_NAME(atan2));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(atan2l));
#endif

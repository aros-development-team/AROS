/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function atan
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double atan(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the arc tangent (inverse tangent) of the input value `x`.

    INPUTS
        x - the value whose arc tangent is to be calculated.

    RESULT
        Returns the arc tangent of `x`, in radians, in the range [-p/2, p/2].

    NOTES
        Used to convert slopes to angles.

    EXAMPLE
        double angle = atan(1.0);  // returns ~0.785 (p/4)

    BUGS
        None known.

    SEE ALSO
        tan(), atan2(), atanf(), atanl()

    INTERNALS
        Forwards to __ieee754_atan().

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(atan, x);
    // entire real line is allowed
    return __ieee754_atan(x);
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(atanl), atanl, AROS_CSYM_FROM_ASM_NAME(atanl), AROS_CSYM_FROM_ASM_NAME(atan));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(atanl));
#endif

/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function hypot
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double hypot(

/*  SYNOPSIS */
        double x,
        double y)

/*  FUNCTION
        Computes sqrt(x*x + y*y) without intermediate overflow or underflow.

    INPUTS
        x, y - double values.

    RESULT
        Hypotenuse sqrt(x² + y²).

    NOTES
        More accurate than direct sqrt(x*x + y*y).

    EXAMPLE
        double h = hypot(3.0, 4.0);  // h = 5.0

    BUGS
        None known.

    SEE ALSO
        hypotf(), hypotl()

    INTERNALS
        Calls __ieee754_hypot(x, y).
        Uses scaling and intermediate normalization.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF2(hypot, x, y);
    double r = __ieee754_hypot(x, y);
    if (!isfinite(r)) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW);
    }
    return r;
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(hypotl), hypotl, AROS_CSYM_FROM_ASM_NAME(hypotl), AROS_CSYM_FROM_ASM_NAME(hypot));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(hypotl));
#endif

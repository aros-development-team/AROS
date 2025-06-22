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
        double x, double y)

/*  FUNCTION
        Computes the length of the hypotenuse of a right triangle with sides
        of length `x` and `y`, without undue overflow or underflow.

    INPUTS
        x - one side of the triangle.
        y - the other side of the triangle.

    RESULT
        Returns sqrt(x² + y²). If the result overflows, sets errno to ERANGE
        and raises FE_OVERFLOW.

    NOTES
        Unlike sqrt(x*x + y*y), `hypot` avoids intermediate overflow/underflow.

    EXAMPLE
        double r = hypot(3.0, 4.0);  // returns 5.0

    BUGS
        None known.

    SEE ALSO
        sqrt(), cabs(), hypotf(), hypotl()

    INTERNALS
        Delegates to __ieee754_hypot() and checks for overflow.

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

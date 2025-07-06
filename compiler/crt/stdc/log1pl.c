/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function cbrtl
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if	LDBL_MANT_DIG != DBL_MANT_DIG
/*****************************************************************************

    NAME */
#include <math.h>

        long double log1pl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes log(1 + x) accurately for small x.

    INPUTS
        x - a number greater than -1.

    RESULT
        Returns log(1 + x).

    NOTES
        Accurate even if x is near zero.

    EXAMPLE
        long double r = log1pl(1.0e-10L);

    BUGS
        None known.

    SEE ALSO
        logl(), expl()

    INTERNALS
        Calls __ieee754_log1pl().

******************************************************************************/
{
    if (x < -1.0L) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    if (x == -1.0L) {
        errno = ERANGE;
        feraiseexcept(FE_DIVBYZERO);
        return -INFINITY;
    }
    return __ieee754_log1pl(x);
}
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

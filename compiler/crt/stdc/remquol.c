/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function remquo
*/

#include <stddef.h>
#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if LDBL_MANT_DIG != DBL_MANT_DIG
/*****************************************************************************

    NAME */
#include <math.h>

long double remquol(

/*  SYNOPSIS */
        long double x, long double y, int *quo)

/*  FUNCTION
        Computes the remainder of dividing x by y and returns part of the
        quotient via `quo`.

    INPUTS
        x   - the dividend.
        y   - the divisor.
        quo - pointer to an int to store part of the quotient.

    RESULT
        Returns the remainder and stores low bits of quotient in *quo.
        If `quo` is NULL or y is zero, returns NaN and sets errno to EDOM.

    NOTES
        The remainder has the same sign as x and magnitude less than y.

    EXAMPLE
        int q;
        double r = remquo(7.0, 2.0, &q);  // r = 1.0, q = 3

    BUGS
        Some platforms may not produce identical quotients for equal inputs.

    SEE ALSO
        remainder(), fmod(), div()

    INTERNALS
        Forwards to __ieee754_remquo(x, y, quo) if valid.

******************************************************************************/
{
    if (!isfinite(x) || !isfinite(y)) {
        return __ieee754_remquol(x, y, quo);
    }
    if (quo == NULL) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    return __ieee754_remquol(x, y, quo);
}
#endif

/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function modf
*/

#include <stddef.h>
#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double modf(

/*  SYNOPSIS */
        double x, double *iptr)

/*  FUNCTION
        Splits the input number into fractional and integer parts.

    INPUTS
        x    - the input value to split.
        iptr - pointer to store the integer part of x.

    RESULT
        Returns the fractional part of x and stores the integer part at *iptr.
        If `iptr` is NULL, returns NaN and sets errno to EDOM.

    NOTES
        The sum of the return value and *iptr equals x (except for rounding).

    EXAMPLE
        double i;
        double f = modf(3.14, &i);  // f = 0.14, i = 3.0

    BUGS
        None known.

    SEE ALSO
        floor(), ceil(), frexp(), modff()

    INTERNALS
        Forwards to __ieee754_modf(x, iptr) and checks for pointer validity.

******************************************************************************/
{
    if (isnan(x)) {
        return __ieee754_modf(x, iptr);
    }
    if (iptr == NULL) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    return __ieee754_modf(x, iptr);
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(modfl), modfl, AROS_CSYM_FROM_ASM_NAME(modfl), AROS_CSYM_FROM_ASM_NAME(modf));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(modfl));
#endif

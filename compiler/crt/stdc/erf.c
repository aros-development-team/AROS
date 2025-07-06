/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function erf
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double erf(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the error function of x.

    INPUTS
        x - input value.

    RESULT
        Returns the probability integral of the Gaussian distribution from 0 to x.

    NOTES
        Used in probability, statistics, and partial differential equations.

    EXAMPLE
        double val = erf(1.0);

    BUGS
        None known.

    SEE ALSO
        erfc(), erfcf(), erfl()

    INTERNALS
        Forwards to __ieee754_erf().

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(erf, x);
    return __ieee754_erf(x);
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(erfl), erfl, AROS_CSYM_FROM_ASM_NAME(erfl), AROS_CSYM_FROM_ASM_NAME(erf));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(erfl));
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */

/* @(#)s_sincos.c 5.1 13/07/15 */
/*
 * ====================================================
 * Copyright (C) 2013 Elliot Saba. All rights reserved.
 *
 * Developed at the University of Washington.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include <aros/debug.h>

#include <float.h>
#include "math.h"
#define INLINE_REM_PIO2
#include "math_private.h"
#include "e_rem_pio2.c"
#include "k_sincos.h"

/****************************************************************************
    NAME
        void sincos(

    SYNOPSIS
        double x, double *sn, double *cs)

    FUNCTION
        Compute the sine and cosine of x.

    INPUTS
        x - Angle in radians.
        sn - Pointer to a writable double for the sine.
        cs - Pointer to a separate writable double for the cosine.

    RESULT
        Store sin(x) in *sn and cos(x) in *cs. No value is returned.
        For positive or negative zero, store x in *sn and 1 in *cs.
        For infinity or NaN, both outputs are NaN.

    NOTES
        Include <math.h>. An infinite argument raises FE_INVALID.
        Enabled floating-point traps may prevent completion.

    BUGS
        Argument reduction assumes round-to-nearest. Other rounding
        modes can produce inaccurate results.

    SEE ALSO
        sin(), cos()

*****************************************************************************/
void
sincos(double x, double *sn, double *cs)
{
    double y[2];
    int32_t n, ix;

	/* High word of x. */
    GET_HIGH_WORD(ix,x);

    /* |x| ~< pi/4 */
    ix &= 0x7fffffff;
    if(ix <= 0x3fe921fb) {
		if (ix < 0x3e400000) {		/* |x| < 2**-27 */
			if ((int)x == 0) {	/* Generate inexact. */
				*sn = x;
				*cs = 1;
				return;
			}
		}
		__kernel_sincos(x, 0, 0, sn, cs);
		return;
	}

	/* If x = Inf or NaN, then sin(x) = NaN and cos(x) = NaN. */
	if (ix >= 0x7ff00000) {
		*sn = x - x;
		*cs = x - x;
		return;
	}

	/* Argument reduction. */
	n = __ieee754_rem_pio2(x, y);

        switch(n&3) {
            case 0:
		__kernel_sincos(y[0], y[1], 1, sn, cs);
                break;
            case 1:
		__kernel_sincos(y[0], y[1], 1, cs, sn);
		*cs = -*cs;
                break;
            case 2:
 		__kernel_sincos(y[0], y[1], 1, sn, cs);
		*sn = -*sn;
		*cs = -*cs;
               break;
            default:
		__kernel_sincos(y[0], y[1], 1, cs, sn);
		*sn = -*sn;
                break;
        }
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(sincosl), sincosl, AROS_CSYM_FROM_ASM_NAME(sincosl), AROS_CSYM_FROM_ASM_NAME(sincos));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(sincosl));
#endif

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

#if (LDBL_MANT_DIG == 53)
AROS_MAKE_ASM_SYM(typeof(sincosl), sincosl, AROS_CSYM_FROM_ASM_NAME(sincosl), AROS_CSYM_FROM_ASM_NAME(sincos));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(sincosl));
#endif

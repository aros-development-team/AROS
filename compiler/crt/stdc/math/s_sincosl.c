/* s_sincosl.c -- long double version of s_sincos.c
 *
 * Copyright (C) 2013 Elliot Saba
 * Developed at the University of Washington
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
*/

#include <aros/debug.h>

#include <float.h>
#include "math.h"
#include "math_private.h"
#include "k_sincosl.h"

#if defined(__x86_64__)
#include "x86_64/ieeefp.h"
#elif defined(__i386__)
#include "i386/ieeefp.h"
#endif

#include "math_private.h"
#if LDBL_MANT_DIG == 64
#include "../ld80/e_rem_pio2l.h"
#elif LDBL_MANT_DIG == 113
#include "../ld128/e_rem_pio2l.h"
#else
#error "Unsupported long double format"
#endif

/****************************************************************************
    NAME
        void sincosl(

    SYNOPSIS
        long double x, long double *sn, long double *cs)

    FUNCTION
        Compute the sine and cosine of x.

    INPUTS
        x - Angle in radians.
        sn - Pointer to a writable long double for the sine.
        cs - Pointer to a separate writable long double for the cosine.

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

        With a 113-bit long-double significand, calls that require
        argument reduction may leave one output unwritten or negate
        its previous value instead of producing the requested result.

    SEE ALSO
        sinl(), cosl()

*****************************************************************************/
void
sincosl(long double x, long double *sn, long double *cs)
{
	union IEEEl2bits z;
	int e0;
	__unused int sgn;
	long double y[2];

	z.e = x;
	sgn = z.bits.sign;
	z.bits.sign = 0;

	ENTERV();

	/* Optimize the case where x is already within range. */
	if (z.e < M_PI_4) {
		/*
		 * If x = +-0 or x is a subnormal number, then sin(x) = x and
		 * cos(x) = 1.
		 */
		if (z.bits.exp == 0) {
			*sn = x;
			*cs = 1;
		} else
			__kernel_sincosl(x, 0, 0, sn, cs);
		RETURNV();
	}

	/* If x = NaN or Inf, then sin(x) and cos(x) are NaN. */
	if (z.bits.exp == 32767) {
		*sn = x - x;
		*cs = x - x;
		RETURNV();
	}

	/* Range reduction. */
	e0 = __ieee754_rem_pio2l(x, y);

	switch (e0 & 3) {
	case 0:
		__kernel_sincosl(y[0], y[1], 1, sn, cs);
		break;
	case 1:
		__kernel_sincosl(y[0], y[1], 1, cs, sn);
		*cs = -*cs;
		break;
	case 2:
		__kernel_sincosl(y[0], y[1], 1, sn, cs);
		*sn = -*sn;
		*cs = -*cs;
		break;
	default:
		__kernel_sincosl(y[0], y[1], 1, cs, sn);
		*sn = -*sn;
	}

	RETURNV();
}

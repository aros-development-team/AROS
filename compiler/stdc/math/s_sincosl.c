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

/* @(#)s_logb.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_logb.c,v 1.12 2008/02/08 01:22:13 bde Exp $";
#endif

/*
 * double logb(x)
 * IEEE 754 logb. Included to pass IEEE test suite. Not recommend.
 * Use ilogb instead.
 */

#include <float.h>
#include "math.h"
#include "math_private.h"

static const double
two54 = 1.80143985094819840000e+16;	/* 43500000 00000000 */

double
logb(double x)
{
	int32_t lx,ix;
	EXTRACT_WORDS(ix,lx,x);
	ix &= 0x7fffffff;			/* high |x| */
	if((ix|lx)==0) return -1.0/fabs(x);
	if(ix>=0x7ff00000) return x*x;
	if(ix<0x00100000) {
		x *= two54;		 /* convert subnormal x to normal */
		GET_HIGH_WORD(ix,x);
		ix &= 0x7fffffff;
		return (double) ((ix>>20)-1023-54);
	} else
		return (double) ((ix>>20)-1023);
}

#if (LDBL_MANT_DIG == DBL_MANT_DIG)
AROS_MAKE_ASM_SYM(typeof(logbl), logbl, AROS_CSYM_FROM_ASM_NAME(logbl), AROS_CSYM_FROM_ASM_NAME(logb));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(logbl));
#endif

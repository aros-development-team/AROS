/* @(#)s_fabs.c 5.1 93/09/24 */
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_fabs.c,v 1.7 2002/05/28 18:15:04 alfred Exp $";
#endif

/*
 * fabs(x) returns the absolute value of x.
 */

#include <float.h>
#include "math.h"
#include "math_private.h"

double
fabs(double x)
{
	uint32_t high;
	GET_HIGH_WORD(high,x);
	SET_HIGH_WORD(x,high&0x7fffffff);
        return x;
}

#if	(LDBL_MANT_DIG == DBL_MANT_DIG)
AROS_MAKE_ASM_SYM(typeof(fabsl), fabsl, AROS_CSYM_FROM_ASM_NAME(fabsl), AROS_CSYM_FROM_ASM_NAME(fabs));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(fabsl));
#endif

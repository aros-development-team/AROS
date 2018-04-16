/* @(#)s_copysign.c 5.1 93/09/24 */
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_copysign.c,v 1.10 2008/02/22 02:30:35 das Exp $";
#endif

/*
 * copysign(double x, double y)
 * copysign(x,y) returns a value with the magnitude of x and
 * with the sign bit of y.
 */

#include <float.h>
#include "math.h"
#include "math_private.h"

double
copysign(double x, double y)
{
	uint32_t hx,hy;
	GET_HIGH_WORD(hx,x);
	GET_HIGH_WORD(hy,y);
	SET_HIGH_WORD(x,(hx&0x7fffffff)|(hy&0x80000000));
        return x;
}

#if LDBL_MANT_DIG == 53
AROS_MAKE_ASM_SYM(typeof(copysignl), copysignl, AROS_CSYM_FROM_ASM_NAME(copysignl), AROS_CSYM_FROM_ASM_NAME(copysign));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(copysignl));
#endif

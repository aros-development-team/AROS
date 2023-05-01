
/* @(#)e_lgamma.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 *
 */

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/e_lgamma.c,v 1.9 2008/02/22 02:30:35 das Exp $";
#endif

/* __ieee754_lgamma(x)
 * Return the logarithm of the Gamma function of x.
 *
 * Method: call __ieee754_lgamma_r
 */

#include <float.h>
#include "math.h"
#include "math_private.h"

#include "__stdc_intbase.h"

double
__ieee754_lgamma(double x)
{
	struct StdCBase *StdCBase = __aros_getbase_StdCBase();
	return __ieee754_lgamma_r(x,&StdCBase->_signgam);
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(lgammal), lgammal, AROS_CSYM_FROM_ASM_NAME(lgammal), AROS_CSYM_FROM_ASM_NAME(lgamma));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(lgammal));
#endif

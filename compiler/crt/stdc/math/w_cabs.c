/*
 * cabs() wrapper for hypot().
 *
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#ifndef lint
static const char rcsid[] =
  "$FreeBSD: src/lib/msun/src/w_cabs.c,v 1.7 2008/03/30 20:03:06 das Exp $";
#endif /* not lint */

#include <float.h>
#include <complex.h>
#include "math.h"

double
cabs(double complex z)
{
	return hypot(creal(z), cimag(z));
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(cabsl), cabsl, AROS_CSYM_FROM_ASM_NAME(cabsl), AROS_CSYM_FROM_ASM_NAME(cabs));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(cabsl));
#endif

/* e_lgammaf.c -- float version of e_lgamma.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/e_lgammaf.c,v 1.8 2008/02/22 02:30:35 das Exp $";
#endif

#include "math.h"
#include "math_private.h"

#include "__stdc_intbase.h"

long double
lgammal(long double x)
{
	struct StdCBase *StdCBase = __aros_getbase_StdCBase();
	return lgammal_r(x,&StdCBase->_signgam);
}

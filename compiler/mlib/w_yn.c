/* from: @(#)w_jn.c 5.1 93/09/24 */
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/w_yn.c,v 1.3 1999/08/28 00:07:11 peter Exp $";
#endif

/*
 * wrapper yn(int n, double x)
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	double yn(int n, double x)	/* wrapper yn */
#else
	double yn(n,x)			/* wrapper yn */
	double x; int n;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_yn(n,x);
#else
	double z;
	z = __ieee754_yn(n,x);
	if(_LIB_VERSION == _IEEE_ || isnan(x) ) return z;
        if(x <= 0.0){
                if(x==0.0)
                    /* d= -one/(x-x); */
                    return __kernel_standard((double)n,x,12);
                else
                    /* d = zero/(x-x); */
                    return __kernel_standard((double)n,x,13);
        }
	if(x>X_TLOSS) {
	    return __kernel_standard((double)n,x,39); /* yn(x>X_TLOSS,n) */
	} else
	    return z;
#endif
}

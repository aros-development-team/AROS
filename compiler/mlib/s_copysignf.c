/* s_copysignf.c -- float version of s_copysign.c.
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_copysignf.c,v 1.9 2003/07/23 04:53:46 peter Exp $";
#endif

/*
 * copysignf(float x, float y)
 * copysignf(x,y) returns a value with the magnitude of x and
 * with the sign bit of y.
 */

#include "math.h"
#include "math_private.h"

float
copysignf(float x, float y)
{
	u_int32_t ix,iy;
	GET_FLOAT_WORD(ix,x);
	GET_FLOAT_WORD(iy,y);
	SET_FLOAT_WORD(x,(ix&0x7fffffff)|(iy&0x80000000));
        return x;
}

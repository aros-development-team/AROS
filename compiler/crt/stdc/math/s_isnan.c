/*-
 * Copyright (c) 2004 David Schultz <das@FreeBSD.ORG>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/lib/msun/src/s_isnan.c,v 1.9 2010/06/12 17:32:05 das Exp $
 */

#include <aros/system.h>
#include <math.h>

#include "fpmath.h"

/****************************************************************************
    NAME
        int __isnan(

    SYNOPSIS
        double d)

    FUNCTION
        Determine whether d is NaN.

    INPUTS
        d - Value to examine.

    RESULT
        Return 1 for NaN; return 0 for finite values and infinity.
        The sign does not affect the result.

    NOTES
        Include <math.h>. Use isnan() for type-generic calls.

    SEE ALSO
        isnan(), __isinf(), __isfinite()

*****************************************************************************/
int
__isnan(double d)
{
	union IEEEd2bits u;

	u.d = d;
	return (u.bits.exp == 2047 && (u.bits.manl != 0 || u.bits.manh != 0));
}

/****************************************************************************
    NAME
        int __isnanf(

    SYNOPSIS
        float f)

    FUNCTION
        Determine whether f is NaN.

    INPUTS
        f - Value to examine.

    RESULT
        Return 1 for NaN; return 0 for finite values and infinity.
        The sign does not affect the result.

    NOTES
        Include <math.h>. Use isnan() for type-generic calls.

    SEE ALSO
        isnan(), __isinff(), __isfinitef()

*****************************************************************************/
int
__isnanf(float f)
{
	union IEEEf2bits u;

	u.f = f;
	return (u.bits.exp == 255 && u.bits.man != 0);
}

/****************************************************************************
    NAME
        int __isnanl(

    SYNOPSIS
        long double e)

    FUNCTION
        Determine whether e is NaN.

    INPUTS
        e - Value to examine.

    RESULT
        Return 1 for NaN; return 0 for finite values and infinity.
        The sign does not affect the result.

    NOTES
        Include <math.h>. Use isnan() for type-generic calls.

    BUGS
        On ARM and PowerPC builds with an 11-bit long-double exponent,
        this function always returns 0.

    SEE ALSO
        isnan(), __isinfl(), __isfinitel()

*****************************************************************************/
int
__isnanl(long double e)
{
	union IEEEl2bits u;

	u.e = e;
	mask_nbit_l(u);
	return (u.bits.exp == 32767 && (u.bits.manl != 0 || u.bits.manh != 0));
}

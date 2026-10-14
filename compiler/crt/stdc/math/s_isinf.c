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
 */

/*
 * isinf(x) returns 1 if x is inf, else 0;
 * no branching!
 */

#include "math.h"
#include "fpmath.h"
#include "math_private.h"

/****************************************************************************
    NAME
        int __isinf(

    SYNOPSIS
        double d)

    FUNCTION
        Determine whether d is infinite.

    INPUTS
        d - Value to examine.

    RESULT
        Return 1 for positive or negative infinity; return 0 for finite
        values and NaN.

    NOTES
        Include <math.h>. Use isinf() for type-generic calls.

    SEE ALSO
        isinf(), __isfinite(), __isnan()

*****************************************************************************/
int
__isinf(double d)
{
	union IEEEd2bits u;

	u.d = d;
	return (u.bits.exp == 2047 && u.bits.manl == 0 && u.bits.manh == 0);
}

/****************************************************************************
    NAME
        int __isinff(

    SYNOPSIS
        float f)

    FUNCTION
        Determine whether f is infinite.

    INPUTS
        f - Value to examine.

    RESULT
        Return 1 for positive or negative infinity; return 0 for finite
        values and NaN.

    NOTES
        Include <math.h>. Use isinf() for type-generic calls.

    SEE ALSO
        isinf(), __isfinitef(), __isnanf()

*****************************************************************************/
int
__isinff(float f)
{
	union IEEEf2bits u;

	u.f = f;
	return (u.bits.exp == 255 && u.bits.man == 0);
}

/****************************************************************************
    NAME
        int __isinfl(

    SYNOPSIS
        long double e)

    FUNCTION
        Determine whether e is infinite.

    INPUTS
        e - Value to examine.

    RESULT
        Return 1 for positive or negative infinity; return 0 for finite
        values and NaN.

    NOTES
        Include <math.h>. Use isinf() for type-generic calls.

    BUGS
        On ARM and PowerPC builds with an 11-bit long-double exponent,
        this function always returns 0.

    SEE ALSO
        isinf(), __isfinitel(), __isnanl()

*****************************************************************************/
int
__isinfl(long double e)
{
	union IEEEl2bits u;

	u.e = e;
	mask_nbit_l(u);
	return (u.bits.exp == 32767 && u.bits.manl == 0 && u.bits.manh == 0);
}

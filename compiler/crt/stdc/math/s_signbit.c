/*-
 * Copyright (c) 2003 Mike Barcroft <mike@FreeBSD.org>
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
 * $FreeBSD: src/lib/msun/src/s_signbit.c,v 1.1 2004/07/19 08:16:10 das Exp $
 */

#include <math.h>

#include "fpmath.h"

/****************************************************************************
    NAME
        int __signbit(

    SYNOPSIS
        double d)

    FUNCTION
        Test the sign bit of d.

    INPUTS
        d - Value to examine.

    RESULT
        Return 1 if the sign bit is set; otherwise return 0.
        This distinguishes negative zero from positive zero and also
        applies to infinities and NaNs.

    NOTES
        Include <math.h>. Use signbit() for type-generic calls.

    SEE ALSO
        signbit(), copysign()

*****************************************************************************/
int
__signbit(double d)
{
	union IEEEd2bits u;

	u.d = d;
	return (u.bits.sign);
}

/****************************************************************************
    NAME
        int __signbitf(

    SYNOPSIS
        float f)

    FUNCTION
        Test the sign bit of f.

    INPUTS
        f - Value to examine.

    RESULT
        Return 1 if the sign bit is set; otherwise return 0.
        This distinguishes negative zero from positive zero and also
        applies to infinities and NaNs.

    NOTES
        Include <math.h>. Use signbit() for type-generic calls.

    SEE ALSO
        signbit(), copysignf()

*****************************************************************************/
int
__signbitf(float f)
{
	union IEEEf2bits u;

	u.f = f;
	return (u.bits.sign);
}

/****************************************************************************
    NAME
        int __signbitl(

    SYNOPSIS
        long double e)

    FUNCTION
        Test the sign bit of e.

    INPUTS
        e - Value to examine.

    RESULT
        Return 1 if the sign bit is set; otherwise return 0.
        This distinguishes negative zero from positive zero and also
        applies to infinities and NaNs.

    NOTES
        Include <math.h>. Use signbit() for type-generic calls.

    SEE ALSO
        signbit(), copysignl()

*****************************************************************************/
int
__signbitl(long double e)
{
	union IEEEl2bits u;

	u.e = e;
	return (u.bits.sign);
}

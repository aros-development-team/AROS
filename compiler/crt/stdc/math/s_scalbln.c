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
 */

__FBSDID("$FreeBSD: src/lib/msun/src/s_scalbln.c,v 1.2 2005/03/07 04:57:50 das Exp $");

#include <limits.h>
#include <math.h>

#define	NMAX	65536
#define	NMIN	-65536

/****************************************************************************
    NAME
        double scalbln(

    SYNOPSIS
        double x, long n)

    FUNCTION
        Scale x by an integral power of two. The exponent is clamped
        to [-65536, 65536] before calling scalbn().

    INPUTS
        x - Double value to scale.
        n - Base-two exponent. Values outside the clamp interval are
            treated as the nearest endpoint.

    RESULT
        The scaled double value. An exact result is returned without
        rounding; otherwise the current rounding mode applies.
        Signed zero retains its sign. Infinities and NaNs propagate.

    NOTES
        Include <math.h>. Overflow and underflow may raise floating-point
        exceptions. Enabled traps may prevent the function from returning.

*****************************************************************************/
double
scalbln (double x, long n)
{
	return (scalbn(x, (n > NMAX) ? NMAX : (n < NMIN) ? NMIN : (int)n));
}

/****************************************************************************
    NAME
        float scalblnf(

    SYNOPSIS
        float x, long n)

    FUNCTION
        Scale x by an integral power of two. The exponent is clamped
        to [-65536, 65536] before calling scalbnf().

    INPUTS
        x - Float value to scale.
        n - Base-two exponent. Values outside the clamp interval are
            treated as the nearest endpoint.

    RESULT
        The scaled float value. An exact result is returned without
        rounding; otherwise the current rounding mode applies.
        Signed zero retains its sign. Infinities and NaNs propagate.

    NOTES
        Include <math.h>. Overflow and underflow may raise floating-point
        exceptions. Enabled traps may prevent the function from returning.

    SEE ALSO
        frexpf()

*****************************************************************************/
float
scalblnf (float x, long n)
{
	return (scalbnf(x, (n > NMAX) ? NMAX : (n < NMIN) ? NMIN : (int)n));
}

/****************************************************************************
    NAME
        long double scalblnl(

    SYNOPSIS
        long double x, long n)

    FUNCTION
        Scale x by an integral power of two. The exponent is clamped
        to [-65536, 65536] before calling scalbnl().

    INPUTS
        x - Long double value to scale.
        n - Base-two exponent. Values outside the clamp interval are
            treated as the nearest endpoint.

    RESULT
        The scaled long double value, subject to the current rounding
        mode and the limitation described in BUGS. Signed zero retains
        its sign. Infinities and NaNs propagate.

    NOTES
        Include <math.h>. Overflow and underflow may raise floating-point
        exceptions. Enabled traps may prevent the function from returning.

    BUGS
        When long double has a separate extended-precision implementation,
        scalbnl() narrows a long double underflow constant through the
        double-precision copysign() function. Under downward rounding,
        a negative input can produce negative zero when a negative
        subnormal result is required. The equal-precision double alias
        does not use this implementation path.

    SEE ALSO
        frexpl()

*****************************************************************************/
long double
scalblnl (long double x, long n)
{
	return (scalbnl(x, (n > NMAX) ? NMAX : (n < NMIN) ? NMIN : (int)n));
}

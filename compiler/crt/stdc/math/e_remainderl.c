/*-
 * Copyright (c) 2008 David Schultz <das@FreeBSD.ORG>
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

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/e_remainderl.c,v 1.1 2008/03/30 20:47:42 das Exp $";
#endif

#include "math.h"
#include "math_private.h"

/****************************************************************************
    NAME
        long double remainderl(

    SYNOPSIS
        long double x, long double y)

    FUNCTION
        Compute x - n * y, where n is the integer nearest to the exact
        quotient x / y. Ties select the even integer, independently of
        the current rounding direction.

    INPUTS
        x - Dividend.
        y - Divisor.

    RESULT
        For finite operands and nonzero y, return a remainder with
        magnitude at most half of |y|. A zero result has the sign of x.
        With finite x and infinite y, return x.

        If x is infinite, y is zero, or either argument is NaN, return NaN.

    NOTES
        Include <math.h>. A zero divisor or infinite dividend raises
        FE_INVALID unless a quiet NaN operand suppresses the exception.
        Enabled traps may prevent return. This function does not set errno.

    BUGS
        Where long double has greater precision than double, results
        are not reliable when the nearest integer quotient has magnitude
        2^31 or greater.

    SEE ALSO
        remainder(), remainderf(), remquol(), fmodl()

*****************************************************************************/
long double
remainderl(long double x, long double y)
{
	int quo;

	return (remquol(x, y, &quo));
}

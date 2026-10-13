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

//__FBSDID("$FreeBSD: src/lib/msun/src/s_cprojl.c,v 1.1 2008/08/07 15:07:48 das Exp $");

#include <complex.h>
#include "math.h"

#include "math_private.h"

/****************************************************************************
    NAME */
        long double complex cprojl(

/*  SYNOPSIS */
        long double complex z)

/*  FUNCTION
        Project z onto the Riemann sphere. If either component is infinite,
        return positive real infinity and an imaginary zero with the sign
        of the original imaginary component. Otherwise return z unchanged.
        This operation does not normalize a finite value to unit magnitude.

    INPUTS
        z - Complex value passed by value. Both signs of infinity and zero,
            and NaN components, are accepted.

    RESULT
        z if neither component is infinite, including values containing a
        NaN but no infinity. If either component is infinite, the result is
        positive real infinity plus a signed imaginary zero, even when the
        other component is NaN. There is no error sentinel.

    NOTES
        Include <complex.h>. The argument and result are values: no storage
        is retained or allocated, and the call performs no I/O or waiting.
        The implementation does not set errno. Floating-point exception
        flags and NaN payload preservation are not specified here.

        The AROS build selects separate long double implementations for
        i386, x86_64 and m68k (80-bit format), and for aarch64, riscv and
        riscv64 (128-bit format). Where long double has the same precision
        as double, the double implementation supplies a symbol alias.
        The l suffix alone does not promise extra precision on every target.

    SEE ALSO
        creall(), cimagl(), conjl()

*****************************************************************************/
{

	if (!isinf(creall(z)) && !isinf(cimagl(z)))
		return (z);
	else
		return (CMPLXL(INFINITY, copysignl(0.0, cimagl(z))));
}

/*-
 * Copyright (c) 2005 David Schultz <das@FreeBSD.ORG>
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

#include <float.h>
#include <fenv.h>
#include <math.h>

#ifndef type
__FBSDID("$FreeBSD: src/lib/msun/src/s_lrint.c,v 1.1 2005/01/11 23:12:55 das Exp $");
#define type		double
#define	roundit		rint
#define dtype		long
#define	fn		lrint
#define	fnld	lrintl
#endif

/*
 * C99 says we should not raise a spurious inexact exception when an
 * invalid exception is raised.  Unfortunately, the set of inputs
 * that overflows depends on the rounding mode when 'dtype' has more
 * significant bits than 'type'.  Hence, we bend over backwards for the
 * sake of correctness; an MD implementation could be more efficient.
 */
/****************************************************************************
    NAME
        long lrint(

    SYNOPSIS
        double x)

    FUNCTION
        Round x in the current floating-point rounding mode and convert
        the result to long. Nearest rounding selects the even integer
        at a tie.

    INPUTS
        x - Finite double value whose rounded result fits in long.

    RESULT
        The integer result. Both signed zeros become zero. No error
        value is reserved for an invalid conversion.

    NOTES
        Include <math.h>. Use <fenv.h> to control the rounding mode.
        With functioning environment support, the wrapper restores the
        caller's environment and clears new FE_INEXACT if FE_INVALID was
        raised. Preexisting exception flags are preserved.

    BUGS
        Conversion of NaN, infinity or an out-of-range rounded value is
        undefined; the wrapper does not validate the integer cast.
        Floating-point environment failures are ignored, and some AROS
        targets provide incomplete or stub implementations. On RISC-V
        hard-float, a failed save can lead to restoration of uninitialized
        environment state.
        The rint() delegate has undefined C behavior for some small
        negative inputs.

    SEE ALSO
        rint(), lround(), llrint(), feholdexcept(), feupdateenv()

*****************************************************************************/
dtype
fn(type x)
{
	fenv_t env;
	dtype d;

	feholdexcept(&env);
	d = (dtype)roundit(x);
	if (fetestexcept(FE_INVALID))
		feclearexcept(FE_INEXACT);
	feupdateenv(&env);
	return (d);
}

#if (LDBL_MANT_DIG == DBL_MANT_DIG) && defined(fnld)
AROS_MAKE_ASM_SYM(typeof(fnld),fnld, AROS_CSYM_FROM_ASM_NAME(fnld), AROS_CSYM_FROM_ASM_NAME(fn));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(fnld));
#endif

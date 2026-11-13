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

__FBSDID("$FreeBSD: src/lib/msun/src/s_nearbyint.c,v 1.2 2008/01/14 02:12:06 das Exp $");

#include <float.h>
#include <fenv.h>
#include <math.h>

/*
 * We save and restore the floating-point environment to avoid raising
 * an inexact exception.  We can get away with using fesetenv()
 * instead of feclearexcept()/feupdateenv() to restore the environment
 * because the only exception defined for rint() is overflow, and
 * rounding can't overflow as long as emax >= p.
 */
#define	DECL(type, fn, rint)	\
type				\
fn(type x)			\
{				\
	volatile type ret;	\
	fenv_t env;		\
				\
	fegetenv(&env);		\
	ret = rint(x);		\
	fesetenv(&env);		\
	return (ret);		\
}

/****************************************************************************
    NAME
        double nearbyint(

    SYNOPSIS
        double x)

    FUNCTION
        Round x to an integral floating-point value according to the
        current rounding direction, without retaining newly
        raised floating-point exception flags when environment
        save and restore succeed.

    INPUTS
        x - Double value to round.

    RESULT
        The value returned by rint(), in double precision.
        Signed zeros and infinities retain their sign; NaNs
        remain NaNs. The result is not a C integer conversion.

    NOTES
        Include <math.h> and <fenv.h>. The wrapper saves the entire
        floating-point environment before rounding and restores
        it afterward. It does not disable enabled traps.

    BUGS
        The wrapper ignores errors from fegetenv() and fesetenv().
        On targets where these operations are ineffective or
        unsupported, exception flags may not be preserved.
        A trap can prevent restoration.
        The delegated rint implementation has undefined signed-shift
        behavior for some small negative inputs.

    SEE ALSO
        rint(), round(), fegetenv(), fesetenv()

*****************************************************************************/
DECL(double, nearbyint, rint)
/****************************************************************************
    NAME
        float nearbyintf(

    SYNOPSIS
        float x)

    FUNCTION
        Round x to an integral floating-point value according to the
        current rounding direction, without retaining newly
        raised floating-point exception flags when environment
        save and restore succeed.

    INPUTS
        x - Float value to round.

    RESULT
        The value returned by rintf(), in float precision.
        Signed zeros and infinities retain their sign; NaNs
        remain NaNs. The result is not a C integer conversion.

    NOTES
        Include <math.h> and <fenv.h>. The wrapper saves the entire
        floating-point environment before rounding and restores
        it afterward. It does not disable enabled traps.

    BUGS
        The wrapper ignores errors from fegetenv() and fesetenv().
        On targets where these operations are ineffective or
        unsupported, exception flags may not be preserved.
        A trap can prevent restoration.
        The delegated rint implementation has undefined signed-shift
        behavior for some small negative inputs.

    SEE ALSO
        rintf(), roundf(), fegetenv(), fesetenv()

*****************************************************************************/
DECL(float, nearbyintf, rintf)
#if (LDBL_MANT_DIG != DBL_MANT_DIG)
/****************************************************************************
    NAME
        long double nearbyintl(

    SYNOPSIS
        long double x)

    FUNCTION
        Round x to an integral floating-point value according to the
        current rounding direction, without retaining newly
        raised floating-point exception flags when environment
        save and restore succeed.

    INPUTS
        x - Long double value to round.

    RESULT
        The value returned by rintl(), in long double precision.
        Signed zeros and infinities retain their sign; NaNs
        remain NaNs. The result is not a C integer conversion.

    NOTES
        Include <math.h> and <fenv.h>. The wrapper saves the entire
        floating-point environment before rounding and restores
        it afterward. It does not disable enabled traps.
        Where long double equals double, nearbyintl() aliases nearbyint().

    BUGS
        The wrapper ignores errors from fegetenv() and fesetenv().
        On targets where these operations are ineffective or
        unsupported, exception flags may not be preserved.
        A trap can prevent restoration.
        The separate rintl() implementation requires correctly set
        long double evaluation precision; equal-precision aliases
        inherit the rint() signed-shift defect.

    SEE ALSO
        rintl(), roundl(), fegetenv(), fesetenv()

*****************************************************************************/
DECL(long double, nearbyintl, rintl)
#else
AROS_MAKE_ASM_SYM(typeof(nearbyintl), nearbyintl, AROS_CSYM_FROM_ASM_NAME(nearbyintl), AROS_CSYM_FROM_ASM_NAME(nearbyint));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(nearbyintl));
#endif
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
        Round x to an integral floating-point value using the current
        rounding direction. The wrapper saves the floating-point
        environment, calls rint(), stores the result in a volatile local
        and restores the saved environment before returning.

        When both environment operations work and the call returns
        normally, exception flags are restored to their prior values,
        including the prior FE_INEXACT state. See BUGS for target limits.

    INPUTS
        x - Double value to round.

    RESULT
        The rint() result, returned as double. With a correctly
        configured floating-point environment, nearest rounding selects
        the even integer at a tie; upward, downward and toward-zero
        rounding use their respective directions. This is not conversion
        to a C integer type.

        Signed zeros retain their sign. Infinities remain infinite and
        NaNs produce NaNs. An enabled trap may prevent normal return.
        There is no separate result indicating environment-save or
        restore failure.

    NOTES
        Include <math.h>; use <fenv.h> to configure the environment.
        Unlike round(), halfway cases do not always round away from
        zero. A later cast to an integer type still needs range checking.

        The wrapper restores the whole saved environment, not just the
        inexact flag. Existing flags are retained after a successful
        normal return; newly raised flags from the delegated calculation
        are discarded by that restoration.

        This wrapper does not mask enabled exceptions before calling
        rint(). Restoration after a trap or nonlocal exit is not
        provided. Signaling-NaN and payload behavior are target-dependent.

        The wrapper has no errno assignment, allocates or retains no
        application storage, and performs no I/O or waiting. Its numerical
        behavior depends on the selected rint implementation and target
        floating-point precision.

    BUGS
        Return codes from fegetenv() and fesetenv() are ignored. If saving
        fails, the local environment may be uninitialized when passed
        to restoration; if restoration fails, new flags may remain set.
        The result does not report either failure.

        The AArch64 inline fegetenv() and fesetenv() in this source tree
        return success without saving or restoring anything. The generic
        external fallback routines return -1 without implementing those
        operations. On such paths, the intended exception-preservation
        contract is not implemented.

        The double and float rint delegates left-shift a signed sign bit
        into bit 31 on their small negative-input paths. That operation
        has undefined behavior in C; successful environment restoration
        does not establish portable numerical behavior for those paths.

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
        Round x to an integral floating-point value using the current
        rounding direction. The wrapper saves the floating-point
        environment, calls rintf(), stores the result in a volatile local
        and restores the saved environment before returning.

        When both environment operations work and the call returns
        normally, exception flags are restored to their prior values,
        including the prior FE_INEXACT state. See BUGS for target limits.

    INPUTS
        x - Float value to round.

    RESULT
        The rintf() result, returned as float. With a correctly
        configured floating-point environment, nearest rounding selects
        the even integer at a tie; upward, downward and toward-zero
        rounding use their respective directions. This is not conversion
        to a C integer type.

        Signed zeros retain their sign. Infinities remain infinite and
        NaNs produce NaNs. An enabled trap may prevent normal return.
        There is no separate result indicating environment-save or
        restore failure.

    NOTES
        Include <math.h>; use <fenv.h> to configure the environment.
        Unlike roundf(), halfway cases do not always round away from
        zero. A later cast to an integer type still needs range checking.

        The wrapper restores the whole saved environment, not just the
        inexact flag. Existing flags are retained after a successful
        normal return; newly raised flags from the delegated calculation
        are discarded by that restoration.

        This wrapper does not mask enabled exceptions before calling
        rintf(). Restoration after a trap or nonlocal exit is not
        provided. Signaling-NaN and payload behavior are target-dependent.

        The wrapper has no errno assignment, allocates or retains no
        application storage, and performs no I/O or waiting. Its numerical
        behavior depends on the selected rint implementation and target
        floating-point precision.

    BUGS
        Return codes from fegetenv() and fesetenv() are ignored. If saving
        fails, the local environment may be uninitialized when passed
        to restoration; if restoration fails, new flags may remain set.
        The result does not report either failure.

        The AArch64 inline fegetenv() and fesetenv() in this source tree
        return success without saving or restoring anything. The generic
        external fallback routines return -1 without implementing those
        operations. On such paths, the intended exception-preservation
        contract is not implemented.

        The double and float rint delegates left-shift a signed sign bit
        into bit 31 on their small negative-input paths. That operation
        has undefined behavior in C; successful environment restoration
        does not establish portable numerical behavior for those paths.

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
        Round x to an integral floating-point value using the current
        rounding direction. The wrapper saves the floating-point
        environment, calls rintl(), stores the result in a volatile local
        and restores the saved environment before returning.

        When both environment operations work and the call returns
        normally, exception flags are restored to their prior values,
        including the prior FE_INEXACT state. See BUGS for target limits.

    INPUTS
        x - Long double value to round.

    RESULT
        The rintl() result, returned as long double. With a correctly
        configured floating-point environment, nearest rounding selects
        the even integer at a tie; upward, downward and toward-zero
        rounding use their respective directions. This is not conversion
        to a C integer type.

        Signed zeros retain their sign. Infinities remain infinite and
        NaNs produce NaNs. An enabled trap may prevent normal return.
        There is no separate result indicating environment-save or
        restore failure.

    NOTES
        Include <math.h>; use <fenv.h> to configure the environment.
        Unlike roundl(), halfway cases do not always round away from
        zero. A later cast to an integer type still needs range checking.

        The wrapper restores the whole saved environment, not just the
        inexact flag. Existing flags are retained after a successful
        normal return; newly raised flags from the delegated calculation
        are discarded by that restoration.

        This wrapper does not mask enabled exceptions before calling
        rintl(). Restoration after a trap or nonlocal exit is not
        provided. Signaling-NaN and payload behavior are target-dependent.

        The wrapper has no errno assignment, allocates or retains no
        application storage, and performs no I/O or waiting. Its numerical
        behavior depends on the selected rint implementation and target
        floating-point precision.

        A separate long double wrapper is emitted when LDBL_MANT_DIG
        differs from DBL_MANT_DIG; otherwise nearbyintl aliases nearbyint.

    BUGS
        Return codes from fegetenv() and fesetenv() are ignored. If saving
        fails, the local environment may be uninitialized when passed
        to restoration; if restoration fails, new flags may remain set.
        The result does not report either failure.

        The AArch64 inline fegetenv() and fesetenv() in this source tree
        return success without saving or restoring anything. The generic
        external fallback routines return -1 without implementing those
        operations. On such paths, the intended exception-preservation
        contract is not implemented.

        The separate rintl source requires intermediate calculations in
        long double precision. This wrapper does not select x87 extended
        precision. A reduced precision setting can therefore invalidate
        the long double rounding result.

        The double and float rint delegates left-shift a signed sign bit
        into bit 31 on their small negative-input paths. That operation
        has undefined behavior in C; successful environment restoration
        does not establish portable numerical behavior for those paths.

    SEE ALSO
        rintl(), roundl(), fegetenv(), fesetenv()

*****************************************************************************/
DECL(long double, nearbyintl, rintl)
#else
AROS_MAKE_ASM_SYM(typeof(nearbyintl), nearbyintl, AROS_CSYM_FROM_ASM_NAME(nearbyintl), AROS_CSYM_FROM_ASM_NAME(nearbyint));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(nearbyintl));
#endif
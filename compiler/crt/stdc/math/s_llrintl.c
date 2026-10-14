/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

__FBSDID("$FreeBSD: src/lib/msun/src/s_llrintl.c,v 1.1 2008/01/14 02:12:06 das Exp $");

#define type            long double
#define roundit         rintl
#define dtype           long long
#define fn              llrintl

/****************************************************************************
    NAME
        long long llrintl(

    SYNOPSIS
        long double x)

    FUNCTION
        Round x in the current floating-point rounding mode and convert
        the result to long long. Nearest rounding selects the even integer
        at a tie.

    INPUTS
        x - Finite long double value whose rounded result fits in long long.

    RESULT
        The integer result. Both signed zeros become zero. No error
        value is reserved for an invalid conversion.

    NOTES
        Include <math.h>. Use <fenv.h> to control the rounding mode.
        With functioning environment support, the wrapper restores the
        caller's environment and clears new FE_INEXACT if FE_INVALID was
        raised. Preexisting exception flags are preserved.
        Where long double has the same precision as double, this symbol
        is an alias of the double-input variant.

    BUGS
        Conversion of NaN, infinity or an out-of-range rounded value is
        undefined; the wrapper does not validate the integer cast.
        Floating-point environment failures are ignored, and some AROS
        targets provide incomplete or stub implementations. On RISC-V
        hard-float, a failed save can lead to restoration of uninitialized
        environment state.
        The separate rintl() delegate depends on long double evaluation
        precision; reduced x87 precision can produce an incorrect result.
        The double alias inherits the double rint() limitation.

    SEE ALSO
        rintl(), lrintl(), llroundl(), feholdexcept(), feupdateenv()

*****************************************************************************/
#include "s_lrint.c"

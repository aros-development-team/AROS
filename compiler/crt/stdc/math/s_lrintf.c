/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

__FBSDID("$FreeBSD: src/lib/msun/src/s_lrintf.c,v 1.1 2005/01/11 23:12:55 das Exp $");

#define type            float
#define roundit         rintf
#define dtype           long
#define fn              lrintf

/****************************************************************************
    NAME
        long lrintf(

    SYNOPSIS
        float x)

    FUNCTION
        Round x in the current floating-point rounding mode and convert
        the result to long. Nearest rounding selects the even integer
        at a tie.

    INPUTS
        x - Finite float value whose rounded result fits in long.

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
        The rintf() delegate has undefined C behavior for some small
        negative inputs.

    SEE ALSO
        rintf(), lroundf(), llrintf(), feholdexcept(), feupdateenv()

*****************************************************************************/
#include "s_lrint.c"

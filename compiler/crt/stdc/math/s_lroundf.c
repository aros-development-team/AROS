/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

__FBSDID("$FreeBSD: src/lib/msun/src/s_lroundf.c,v 1.2 2005/04/08 00:52:27 das Exp $");

#define type            float
#define roundit         roundf
#define dtype           long
#define DTYPE_MIN       LONG_MIN
#define DTYPE_MAX       LONG_MAX
#define fn              lroundf

/****************************************************************************
    NAME
        long lroundf(

    SYNOPSIS
        float x)

    FUNCTION
        Round x to the nearest integer, with halfway cases away from
        zero, and return the result as long. The current rounding
        direction does not affect the result.

    INPUTS
        x - Finite float value whose rounded result fits in long.

    RESULT
        The rounded integer. Both signs of zero return 0. No defined
        result is guaranteed for nonfinite inputs or an out-of-range
        conversion.

    NOTES
        Include <math.h>. The implementation calls roundf() before
        conversion to long. It does not assign errno.
        Rounding can affect floating-point exception flags.

    BUGS
        The range check can accept NaNs, infinities and out-of-range
        values, allowing undefined floating-to-integer conversions.
        The explicit rejection path raises FE_INVALID and returns LONG_MAX
        if execution continues, but not all invalid inputs reach it.
        LONG_MAX is also a valid result, not a unique error sentinel.

    SEE ALSO
        roundf(), llroundf(), feraiseexcept()

*****************************************************************************/
#include "s_lround.c"

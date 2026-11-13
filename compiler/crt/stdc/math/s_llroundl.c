/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

__FBSDID("$FreeBSD: src/lib/msun/src/s_llroundl.c,v 1.1 2005/04/08 01:24:08 das Exp $");

#define type            long double
#define roundit         roundl
#define dtype           long long
#define DTYPE_MIN       LLONG_MIN
#define DTYPE_MAX       LLONG_MAX
#define fn              llroundl

/****************************************************************************
    NAME
        long long llroundl(

    SYNOPSIS
        long double x)

    FUNCTION
        Round x to the nearest integer, with halfway cases away from
        zero, and return the result as long long. The current rounding
        direction does not affect the result.

    INPUTS
        x - Finite long double value whose rounded result fits in long long.

    RESULT
        The rounded integer. Both signs of zero return 0. No defined
        result is guaranteed for nonfinite inputs or an out-of-range
        conversion.

    NOTES
        Include <math.h>. The implementation calls roundl() before
        conversion to long long. It does not assign errno.
        Rounding can affect floating-point exception flags.

        On targets where long double and double have equal precision,
        this symbol aliases the corresponding double implementation.

    BUGS
        The range check can accept NaNs, infinities and out-of-range
        values, allowing undefined floating-to-integer conversions.
        The explicit rejection path raises FE_INVALID and returns LLONG_MAX
        if execution continues, but not all invalid inputs reach it.
        LLONG_MAX is also a valid result, not a unique error sentinel.

    SEE ALSO
        roundl(), lroundl(), feraiseexcept()

*****************************************************************************/
#include "s_lround.c"

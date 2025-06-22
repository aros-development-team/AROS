/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function fmodl
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if (LDBL_MANT_DIG != 53)
/* fmodl */
long double fmodl(long double x, long double y) {
    FORWARD_IF_NAN_OR_INF2(fmodl, x, y);

    // Handle special cases: y == 0 or x infinite ? domain error
    if (y == 0.0 || !isfinite(x)) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return __builtin_nan("");  // or (x - x) as NaN
    }

    double r = __ieee754_fmodl(x, y);

    // fmod should never overflow, so no ERANGE check here

    return r;
}
#endif

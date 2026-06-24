/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    x86_64 floating point environment start-up for the C runtime.
*/

#include <exec/types.h>
#include <aros/symbolsets.h>

#include <fenv.h>

/* Marker symbol referenced by the (architecture independent) stdc start-up
   code so that this module is pulled into every executable linked against
   stdc. */
ULONG __stdc_mathinit = 0;

/*
 * C99 7.6.1: on program start-up the floating point environment is in its
 * default state, which in particular means the rounding direction is
 * round-to-nearest. A freshly created AROS task may bring the FPU up with a
 * different rounding direction (the x87 unit in particular may default to
 * round-toward-zero), which breaks long double rint()/nearbyint()/lrint() and
 * the accuracy of the l-suffixed math routines, so establish it here.
 *
 * On x86_64 SSE is part of the baseline ABI, so fesetround() unconditionally
 * configures both the x87 control word (used for long double) and the SSE
 * MXCSR (used for float/double) - no run-time SSE detection is involved.
 */
static int __stdc_setdefaultround(void)
{
    fesetround(FE_TONEAREST);

    return 1;
}

ADD2INIT(__stdc_setdefaultround, 0);

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    i386 floating point environment start-up for the C runtime.
*/

#include <exec/types.h>
#include <aros/symbolsets.h>

/* Marker symbol referenced by the (architecture independent) stdc start-up
   code so that this module is pulled into every executable linked against
   stdc. */
ULONG __stdc_mathinit = 0;

/*
 * C99 7.6.1: on program start-up the floating point environment is in its
 * default state, which in particular means the rounding direction is
 * round-to-nearest. A freshly created AROS task may bring the x87 unit up
 * rounding toward zero, which breaks long double rint()/nearbyint()/lrint()
 * and the accuracy of the l-suffixed math routines, so establish it here.
 *
 * Only the x87 control word is touched: the SSE/MXCSR rounding default is
 * already round-to-nearest, and going through fesetround() would drag the
 * i386 SSE runtime detection (__has_sse/__test_sse) into every executable's
 * start-up.
 */
static int __stdc_setdefaultround(void)
{
    unsigned short cw;

    __asm__ __volatile__("fnstcw %0" : "=m"(cw));
    cw &= ~0x0c00;  /* clear the rounding control field -> round to nearest */
    __asm__ __volatile__("fldcw %0" : : "m"(cw));

    return 1;
}

ADD2INIT(__stdc_setdefaultround, 0);

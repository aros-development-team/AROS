/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Shared core for the arc4random() family.  The real work - producing
    cryptographically-strong bytes - is delegated to entropy.resource, which
    is treated as an optional library: it is opened and cached in the stdc
    base (see __entropy_available()) and only used when present.  If it is
    unavailable a self-contained xorshift fallback keeps arc4random() usable
    (it cannot report failure), seeded from whatever cheap system state we can
    reach.  Only this file needs to know how the bytes are obtained; the three
    public entry points build on __arc4random_buf().
*/

#include <proto/exec.h>
#include <proto/entropy.h>

#include <stddef.h>
#include <time.h>

#include "__stdc_intbase.h"
#include "__optionallibs.h"
#include "__arc4random.h"

/* Route entropy.resource calls through the base cached in the stdc library
   base (mirrors time.c's handling of TimerBase). */
#define EntropyBase     StdCBase->StdCEntropyBase

/* Degraded, best-effort fallback used only when entropy.resource is not
   present.  A xorshift128 seeded from time, task and address state, decorre-
   lated across calls by a per-base counter. */
static void arc4_fallback(struct StdCIntBase *StdCBase, UBYTE *p, size_t n)
{
    UBYTE  here;
    ULONG  x = (ULONG)time(NULL) ^ (ULONG)(IPTR)&here;
    ULONG  y = (ULONG)(IPTR)StdCBase ^ (ULONG)(IPTR)FindTask(NULL);
    ULONG  z = 0x9e3779b9UL ^ (ULONG)(++StdCBase->arc4_fallbackctr);
    ULONG  w = (x << 13) ^ (y >> 7) ^ (z << 3) ^ 0x2545f491UL;

    while (n > 0)
    {
        ULONG t = x ^ (x << 11);
        size_t take = (n < 4) ? n : 4;
        size_t i;

        x = y; y = z; z = w;
        w = w ^ (w >> 19) ^ (t ^ (t >> 8));

        for (i = 0; i < take; i++)
            p[i] = (UBYTE)(w >> (i * 8));
        p += take;
        n -= take;
    }
}

void __arc4random_buf(struct StdCIntBase *StdCBase, void *buf, size_t n)
{
    UBYTE *p = (UBYTE *)buf;

    if (n == 0)
        return;

    if (__entropy_available(StdCBase))
    {
        while (n > 0)
        {
            /* GetEntropy() takes a ULONG length; chunk very large requests. */
            ULONG chunk = (n > 0x40000000UL) ? 0x40000000UL : (ULONG)n;
            LONG  got   = GetEntropy(p, chunk);

            if (got <= 0)
                break;                  /* fall back for whatever remains */

            p += got;
            n -= (size_t)got;
        }
    }

    if (n > 0)
        arc4_fallback(StdCBase, p, n);
}

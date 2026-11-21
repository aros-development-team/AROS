/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    kaffeine-jit68.library: the one function of the library, and what the
    engine needs from the C library that a library module does not get.
*/
#include <aros/libcall.h>
#include <exec/alerts.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <stdlib.h>
#include <time.h>

#include "kj68_intern.h"

/* LVO -30, the first function after the standard vectors: the engine's
   table of operations for an interface major, or NULL. */
AROS_LH1(APTR, KJ68_GetEngine,
         AROS_LHA(ULONG, abiMajor, D0),
         struct Library *, KJ68Base, 5, KJ68)
{
    AROS_LIBFUNC_INIT

    return (APTR)KJ68_GetEngineTable(abiMajor);

    AROS_LIBFUNC_EXIT
}

/* The translator calls abort() where an AArch64 encoding it relies on does
   not exist, which its self test rules out, and clock_gettime() for a
   counter of translation time. A library module is linked with the static
   C library, which has neither. Here the first is a dead end alert and the
   second reports that it has no clock, which leaves that counter at 0. */
void abort(void)
{
    Alert(AT_DeadEnd | AN_Unknown);
    for (;;)
        ;
}

int clock_gettime(clockid_t clock, struct timespec *ts)
{
    (void)clock;
    (void)ts;
    return -1;
}

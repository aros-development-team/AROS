/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/startup.h>
#include <libraries/stdc.h>

#include "debug.h"

/* The floating point environment start-up (which, on architectures that need
   it, establishes the C standard default rounding direction) lives in its own
   link-library module - __stdc_mathinit.c - and registers itself through
   ADD2INIT(). Reference the module's marker symbol here: stdc_startup.c is
   linked into every executable that uses stdc, so this pulls the module (and
   hence its ADD2INIT() handler) in. Architectures that need no set-up provide
   a do-nothing module exporting the same symbol. */
extern ULONG __stdc_mathinit;
ULONG *const __stdc_mathinit_ref = &__stdc_mathinit;

static void __stdc_startup(struct ExecBase *SysBase)
{
    jmp_buf exitjmp;

    if (setjmp(exitjmp) == 0)
    {
        D(bug("[%s] %s: setjmp() called\n", STDCNAME, __func__));

        /* Tell stdc.library a program using it has started */
        __stdc_program_startup(exitjmp, (int *)&__startup_error);
        D(bug("[__stdc_startup] Library startup called\n", STDCNAME, __func__));

        __startup_entries_next();
    }
    else
    {
        D(bug("[%s] %s: setjmp() return from longjmp\n", STDCNAME, __func__));
    }

    /* Tell stdc.library program has reached the end */
    __stdc_program_end();

    D(bug("[%s] %s: Leave\n", STDCNAME, __func__));
}

ADD2SET(__stdc_startup, PROGRAM_ENTRIES, 0);

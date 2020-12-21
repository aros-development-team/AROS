/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/startup.h>
#include <libraries/stdc.h>

#include "debug.h"

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

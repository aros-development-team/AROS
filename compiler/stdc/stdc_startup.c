/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/startup.h>

#include "__stdc_intbase.h"

static void __stdc_startup(struct ExecBase *SysBase)
{
    jmp_buf exitjmp;

    if (setjmp(exitjmp) == 0)
    {
        D(bug("[__stdc_startup] setjmp() called\n"));

        /* Tell stdc.library a program using it has started */
        __stdc_program_startup(exitjmp, (int *)&__startup_error);
        D(bug("[__stdc_startup] Library startup called\n"));

        __startup_entries_next();
    }
    else
    {
        D(bug("[__stdc_startup] setjmp() return from longjmp\n"));
    }

    /* Tell stdc.library program has reached the end */
    __stdc_program_end();

    D(bug("[__stdc_startup] Leave\n"));
}

ADD2SET(__stdc_startup, PROGRAM_ENTRIES, 0);

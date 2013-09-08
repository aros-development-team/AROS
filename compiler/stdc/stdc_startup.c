/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - arosc.library specific code
    Lang: english
*/

#include <libraries/stdc.h>

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/startup.h>

#include "__arosc_privdata.h"

static int __arosc_init(void)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();

    aroscbase->StdCBase = __aros_getbase_StdCBase();

    return aroscbase->StdCBase != NULL;
}

static void __arosc_startup(struct ExecBase *SysBase)
{
    jmp_buf exitjmp;

    if (setjmp(exitjmp) == 0)
    {
        D(bug("[__arosc_startup] setjmp() called\n"));

        /* Tell arosc.library a program using it has started */
        __arosc_program_startup(exitjmp, &__startup_error);
        D(bug("[__arosc_startup] Library startup called\n"));

        __startup_entries_next();
    }
    else
    {
        D(bug("[__arosc_startup] setjmp() return from longjmp\n"));
    }

    /* Tell arosc.library program has reached the end */
    __arosc_program_end();

    D(bug("[__arosc_startup] Leave\n"));
}

ADD2INIT(__arosc_init, 0);
ADD2SET(__arosc_startup, PROGRAM_ENTRIES, 0);

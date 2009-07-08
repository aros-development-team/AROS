/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - arosc.library specific code
    Lang: english
*/

#include <aros/symbolsets.h>
#include <aros/startup.h>

#define DEBUG 0
#include <aros/debug.h>

#include "arosc_init.h"
#include "etask.h"

static int __arosc_libopen(struct Library *aroscbase)
{
    return arosc_internalinit();
}

static void __arosc_libclose(struct Library *aroscbase)
{
    arosc_internalexit();
}

ADD2OPENLIB(__arosc_libopen, 0);
ADD2CLOSELIB(__arosc_libclose, 0);

ADD2LIBS("arosc.library", 39, struct Library *, aroscbase);

static struct arosc_startup arosc_startup;

static void __arosc_startup(void)
{
    struct Process *myproc;

    D(bug("[__arosc_startup] Start\n"));

    myproc = (struct Process *)FindTask(NULL);
    GetIntETask(FindTask(NULL))->iet_startup = &arosc_startup;
    arosc_startup.as_startup_error = __startup_error;

    if (setjmp(arosc_startup.as_startup_jmp_buf) == 0)
    {
        D(bug("[__arosc_startup] setjmp() called\n"));
        __startup_entries_next();
    }
    else
    {
        D(bug("[__arosc_startup] setjmp() return from longjmp\n"));
        __startup_error = arosc_startup.as_startup_error;
    }

    D(bug("[__arosc_startup] Leave\n"));
}

ADD2SET(__arosc_startup, program_entries, 0);

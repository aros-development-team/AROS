/*
    Copyright � 1995-2009, The AROS Development Team. All rights reserved.
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

void __arosc_program_startup(void);
void __arosc_program_end(void);


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

ADD2LIBS("arosc.library", 42, struct Library *, aroscbase);

static struct arosc_startup arosc_startup;

static void __arosc_startup(void)
{
    D(bug("[__arosc_startup] Start\n"));

    __arosc_program_startup();

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

    __arosc_program_end();

    D(bug("[__arosc_startup] Leave\n"));
}

ADD2SET(__arosc_startup, program_entries, 0);

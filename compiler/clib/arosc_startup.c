/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - arosc.library specific code
    Lang: english
*/
#include <proto/arosc.h>

#include <aros/symbolsets.h>
#include <aros/startup.h>

#define DEBUG 0
#include <aros/debug.h>

#include "__arosc_privdata.h"

void __arosc_program_startup(void);
void __arosc_program_end(void);

static void __arosc_startup(void)
{
    D(bug("[__arosc_startup] Start\n"));

    __arosc_program_startup();

    GetIntETask(FindTask(NULL))->iet_startup = AllocMem(sizeof(struct arosc_startup), MEMF_PUBLIC);
    __aros_startup_error = __startup_error;

    if (setjmp(__aros_startup_jmp_buf) == 0)
    {
        D(bug("[__arosc_startup] setjmp() called\n"));
        __startup_entries_next();
    }
    else
    {
        D(bug("[__arosc_startup] setjmp() return from longjmp\n"));
        __startup_error = __aros_startup_error;
    }

    FreeMem(__aros_startup, sizeof(struct arosc_startup));
    GetIntETask(FindTask(NULL))->iet_startup = NULL;

    __arosc_program_end();

    D(bug("[__arosc_startup] Leave\n"));
}

ADD2SET(__arosc_startup, program_entries, 0);

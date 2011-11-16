/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - arosc.library specific code
    Lang: english
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/startup.h>

#include <sys/arosc.h>

static void __arosc_startup(void)
{
    struct arosc_userdata *udata = __get_arosc_userdata();

    D(bug("[__arosc_startup] Start, base 0x%p, userdata 0x%p\n", aroscbase, udata));

    __arosc_program_startup();

    D(bug("[__arosc_startup] Library startup called, error %d\n", __startup_error));
    udata->acud_startup_error = __startup_error;

    if (setjmp(udata->acud_startup_jmp_buf) == 0)
    {
        D(bug("[__arosc_startup] setjmp() called\n"));
        __startup_entries_next();
    }
    else
    {
        D(bug("[__arosc_startup] setjmp() return from longjmp\n"));
        __startup_error = udata->acud_startup_error;
    }

    __arosc_program_end();

    D(bug("[__arosc_startup] Leave\n"));
}

ADD2SET(__arosc_startup, program_entries, 0);

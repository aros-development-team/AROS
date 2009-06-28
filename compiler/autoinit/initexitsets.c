/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - handle init and exit symolsets
*/
#include <aros/startup.h>
#include <aros/symbolsets.h>
#include <setjmp.h>

#define DEBUG 0
#include <aros/debug.h>

int __noinitexitsets __attribute__((weak)) = 0;

DEFINESET(CTORS);
DEFINESET(DTORS);
DEFINESET(INIT);
DEFINESET(EXIT);

static void __startup_initexit(void)
{
    D(bug("Entering __startup_initexit\n"));

    if (set_open_libraries())
    {
        if (set_call_funcs(SETNAME(INIT), 1, 1))
	{
            /* ctors/dtors get called in inverse order than init funcs */
            set_call_funcs(SETNAME(CTORS), -1, 0);

            __startup_entries_next();

            set_call_funcs(SETNAME(DTORS), 1, 0);
        }
        set_call_funcs(SETNAME(EXIT), -1, 0);
    }
    set_close_libraries();
    
    D(bug("Leaving __startup_initexit\n"));
}

ADD2SET(__startup_initexit, program_entries, -20);

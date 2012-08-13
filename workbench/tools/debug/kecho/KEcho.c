/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:
*/

/******************************************************************************

    NAME

        KEcho [<string>] [NOLINE]

    SYNOPSIS

        STRINGS/M,NOLINE/S

    LOCATION

       SYS:tests

    FUNCTION

        Appends one or more strings to the debug output.

    INPUTS

        STRINGS -- the strings to display
        NOLINE -- no newline after the last string

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/execbase.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>

#define SH_GLOBAL_SYSBASE       1       /* for kprintf() */
#include <aros/shcommands.h>

AROS_SH2(KEcho, 41.1,
AROS_SHA(STRPTR *, , STRINGS, /M,   NULL),
AROS_SHA(BOOL,     , NOLINE, /S,   FALSE))
{
    AROS_SHCOMMAND_INIT

    LONG error = 0;
    STRPTR *a;

    a = SHArg(STRINGS);

    if (a) while (*a != NULL)
    {
        kprintf("%s", *a);
	a++;
        if (*a)
            kprintf(" ");
    }

    if (!SHArg(NOLINE))
        kprintf("\n");

    return error;

    AROS_SHCOMMAND_EXIT
}


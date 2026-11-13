/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Get the name of the current program.
*/
#include <aros/debug.h>

#include <proto/exec.h>
#include <dos/dos.h>
#include "dos_intern.h"
#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, GetProgramName,

/*  SYNOPSIS */
        AROS_LHA(STRPTR, buf, D1),
        AROS_LHA(LONG  , len, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 96, Dos)

/*  FUNCTION
        Copies the name of the current program from the CLI structure
        into the buffer. If the buffer is too small the name is truncated,
        and a failure is returned. If the current process doesn't have
        a CLI structure, a 0 length string is put into the buffer and a
        failure is returned.

    INPUTS
        buf - Buffer for the name.
        len - Size of the buffer in bytes.

    RESULT
        !=0 on success, 0 on failure. IoErr() gives additional information
        in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SetProgramName()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    struct CommandLineInterface *cli;

    ASSERT_VALID_PROCESS(me);

    cli = BADDR(me->pr_CLI);
    if (cli == NULL)
    {
        *buf = '\0';
        me->pr_Result2 = ERROR_OBJECT_WRONG_TYPE;
        return DOSFALSE;
    }

    return internal_GetCliString(me, cli->cli_CommandName, buf, len);
    AROS_LIBFUNC_EXIT
} /* GetProgramName */

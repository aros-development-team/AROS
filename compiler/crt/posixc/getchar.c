/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    C99 function getchar().
*/

#include <aros/debug.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__fdesc.h"

#include "__posixc_intbase.h"

#define _STDIO_H_NOMACRO
#include <stdio.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

        int getchar (void)

/*  SYNOPSIS */

/*  FUNCTION
        Read one character from the standard input stream. If there
        is no character available or an error occurred, the function
        returns EOF.

    INPUTS

    RESULT
        The character read or EOF on end of file or error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        fgetc(), getc(), fputc(), putc()

    INTERNALS

******************************************************************************/
{
    struct PosixCBase *PosixCBase =
        (struct PosixCBase *)__aros_getbase_PosixCBase();
    int retval;

    D(
        bug("[POSIXC] %s(0x%p)\n", __func__, PosixCBase);
        bug("[POSIXC] %s: stdin @ 0x%p\n", __func__, PosixCBase->_stdin);
    )

    retval = fgetc(PosixCBase->_stdin);

    D(bug("[POSIXC] %s: returning %08x\n", __func__, retval);)

    return retval;
} /* getc */


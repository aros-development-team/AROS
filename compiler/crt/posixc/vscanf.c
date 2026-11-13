/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    C99 function vscanf()
*/

#include <libraries/posixc.h>

#include <stdarg.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

        int vscanf (

/*  SYNOPSIS */
        const char * format,
        va_list      args)

/*  FUNCTION
        Reads input from the standard input stream (`stdin`), interprets it
        according to the provided format string, and stores the results in
        the locations specified by the variable argument list.

    INPUTS
        format - A `scanf`-style format string specifying how to interpret the input.
        args   - A `va_list` containing pointers to variables where the converted values
                 should be stored.

    RESULT
        Returns the number of input items successfully matched and assigned.
        Returns `EOF` if an input failure occurs before any conversion.

    NOTES
        - This function is the `va_list` variant of `scanf()`.
        - Input is read from the standard input stream obtained from
          `PosixCBase->_stdin`.
        - The actual scanning is performed by `vfscanf()`.

    EXAMPLE
        va_list ap;
        va_start(ap, fmt);
        vscanf("%d %s", ap);
        va_end(ap);

    BUGS
        - Assumes `PosixCBase` and `_stdin` are properly initialized.
        - Behavior is undefined if arguments do not match the format string.

    SEE ALSO
        scanf(), fscanf(), vfscanf(), gets(), fgets()

    INTERNALS
        - Retrieves the standard input stream from `PosixCBase->_stdin`.
        - Delegates parsing and conversion to `vfscanf()`.

******************************************************************************/
{
    struct PosixCBase *PosixCBase = __aros_getbase_PosixCBase();

    return vfscanf (PosixCBase->_stdin, format, args);
} /* vscanf */


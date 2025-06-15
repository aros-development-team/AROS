/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    C99 function scanf().
*/

#include <libraries/posixc.h>

#include <stdarg.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

        int scanf (

/*  SYNOPSIS */
        const char * format,
        ...)

/*  RESULT
        Returns the number of input items successfully matched and assigned.
        This number can be less than the number requested, or even zero, if
        a matching failure occurs before any assignments. If an input failure
        occurs before any conversions, EOF is returned.

    NOTES
        - The function reads from the standard input stream (stdin).
        - The behavior and supported format specifiers conform to the C
          standard library specification.
        - It is recommended to check the return value to detect input errors
          or mismatches.

    EXAMPLE
        int x;
        float y;
        scanf("%d %f", &x, &y);

    BUGS
        - Input matching is dependent on the format string correctness.
        - Mismatched format specifiers and argument types can lead to
          undefined behavior.

    SEE ALSO
        fscanf(), vscanf(), vfscanf(),
        stdc.library/sscanf(), stdc.library/vsscanf()

    INTERNALS
        This function calls `vfscanf()` on the standard input stream, passing
        the format string and the variable argument list for processing.

******************************************************************************/
{
    struct PosixCBase *PosixCBase = __aros_getbase_PosixCBase();
    int     retval;
    va_list args;

    va_start (args, format);

    retval = vfscanf (PosixCBase->_stdin, format, args);

    va_end (args);

    return retval;
} /* scanf */


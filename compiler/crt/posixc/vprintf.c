/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    C99 function vprintf()
*/

#include <libraries/posixc.h>

#include <stdarg.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

        int vprintf (

/*  SYNOPSIS */
        const char * format,
        va_list      args)

/*  FUNCTION
        Writes formatted output to the standard output stream using a
        variable argument list. This function is the `va_list`-based
        equivalent of `printf()`.

    INPUTS
        format - A `printf`-style format string specifying how to format the output.
        args   - A `va_list` containing the arguments to format and print.

    RESULT
        Returns the number of characters printed, or a negative value if an error occurs.

    NOTES
        - This function uses the AROS-specific global pointer `PosixCBase` to access
          `_stdout`, the standard output stream.
        - `vprintf()` is implemented by calling `vfprintf()` with `stdout`.

    EXAMPLE
        va_list ap;
        va_start(ap, fmt);
        vprintf("%s: %d\n", ap);
        va_end(ap);

    BUGS
        - Assumes `PosixCBase` and `_stdout` are valid; if not, undefined behavior may occur.
        - Limited to the formatting capabilities of `vfprintf()`.

    SEE ALSO
        printf(), fprintf(), vfprintf(), vsprintf(), puts(), fputs()

    INTERNALS
        - Retrieves the standard output stream from `PosixCBase->_stdout`.
        - Delegates actual output formatting and writing to `vfprintf()`.

******************************************************************************/
{
    struct PosixCBase *PosixCBase = __aros_getbase_PosixCBase();

    return vfprintf (PosixCBase->_stdout, format, args);
} /* vprintf */


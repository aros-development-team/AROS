/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function fputws().
*/

#include <wchar.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

        wint_t fputws (

/*  SYNOPSIS */
        const wchar_t *ws,
        FILE *stream)

/*  FUNCTION
        Write a wide-character string to the specified stream.
        Converts each wide character to UTF-8 and writes the sequence of bytes.

    INPUTS
        ws - Pointer to the null-terminated wide-character string to write.
        stream - Pointer to the output stream.

    RESULT
        Returns a non-negative number on success, or WEOF on error.

    NOTES
        The null terminator is not written.
        Uses fputwc internally for each wide character.

    EXAMPLE

    BUGS

    SEE ALSO
        fputwc(), fwrite()

    INTERNALS

******************************************************************************/
{
    while (*ws != L'\0') {
        if (fputwc(*ws++, stream) == WEOF) {
            return WEOF;
        }
    }
    return 0;  // success, usually 0 or a non-negative number
}

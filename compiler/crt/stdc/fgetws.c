/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function fgetws().
*/

#include <wchar.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

        wchar_t *fgetws (

/*  SYNOPSIS */
        wchar_t *ws,
        int n,
        FILE *stream)

/*  FUNCTION
        Reads a wide-character string from the specified stream.
        Converts UTF-8 input bytes to wide characters and stores up to n-1 wide characters.
        Null-terminates the string.

    INPUTS
        ws - Buffer to store the wide string.
        n - Maximum number of wide characters to read (including null terminator).
        stream - Input stream to read from.

    RESULT
        Returns ws on success, or NULL on error or end of file.

    NOTES
        Handles UTF-8 decoding and stream buffering.

    EXAMPLE

    BUGS

    SEE ALSO
        fgets(), fgetwc()

    INTERNALS

******************************************************************************/
{
    if (ws == NULL || n <= 0 || stream == NULL) {
        errno = EINVAL;
        return NULL;
    }

    int count = 0;
    wint_t wc;

    // Read until buffer full (n-1 to leave space for null terminator)
    while (count < n - 1) {
        wc = fgetwc(stream);
        if (wc == WEOF) {
            if (count == 0) // nothing read and EOF
                return NULL;
            break;
        }

        ws[count++] = (wchar_t)wc;

        if (wc == L'\n') // stop at newline
            break;
    }

    ws[count] = L'\0'; // null-terminate

    return ws;
}

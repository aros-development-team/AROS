/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function ungetwc().
*/

#include <wchar.h>
#include <errno.h>

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        wint_t ungetwc (

/*  SYNOPSIS */
        wint_t wc,
        FILE * stream)

/*  FUNCTION
        Push a wide character back onto the input stream.
        The next read operation will return this character.

    INPUTS
        wc - Wide character to push back (can be WEOF).
        stream - Pointer to the stream.

    RESULT
        Returns the wide character pushed back, or WEOF on error.

    NOTES
        Only one wide character can be pushed back at a time.
        Subsequent calls without reading will overwrite the previous pushed-back character.

    EXAMPLE

    BUGS

    SEE ALSO
        fgetwc(), fputwc()

    INTERNALS

******************************************************************************/
{
    if (!stream) {
        errno = EINVAL;
        return WEOF;
    }

    // If there's already a pushed-back wide char, fail (only one supported)
    if (stream->flags & __STDCIO_STDIO_UNGETWC)
        return WEOF;

    // Validate input character
    if (wc == WEOF)
        return WEOF;

    // Store the pushed-back character and set flag
    stream->unget_wc = wc;
    stream->flags |= __STDCIO_STDIO_UNGETWC;

    return wc;
}

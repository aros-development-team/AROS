/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    C99 function fwide().
*/

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        int fwide (

/*  SYNOPSIS */
        FILE *stream,
        int mode)

/*  FUNCTION
        Sets or queries the orientation of the given stream for wide or byte I/O.
        If mode > 0, sets stream orientation to wide-character.
        If mode < 0, sets stream orientation to byte-oriented.
        If mode == 0, returns current orientation without changing it.

    INPUTS
        stream - The stream to query or set orientation.
        mode - Orientation mode to set or 0 to query.

    RESULT
        Returns positive value if stream is wide-oriented,
        negative if byte-oriented,
        or 0 if no orientation is set.

    NOTES
        Orientation is locked after first I/O operation.

    EXAMPLE

    BUGS

    SEE ALSO
        fgetwc(), fputwc()

    INTERNALS

******************************************************************************/
{
    if (!stream)
        return 0;

    /* Determine the current orientation: 0 = none, 1 = wide, -1 = byte. */
    int orientation = 0;
    if (stream->flags & __STDCIO_STDIO_WIDE)
        orientation = 1;
    else if (stream->flags & __STDCIO_STDIO_BYTE)
        orientation = -1;

    /* A non-zero mode only sets the orientation of a stream that has not
       yet been oriented; an already oriented stream is never changed. */
    if (mode != 0 && orientation == 0) {
        if (mode > 0) {
            stream->flags |= __STDCIO_STDIO_WIDE;
            orientation = 1;
        } else {
            stream->flags |= __STDCIO_STDIO_BYTE;
            orientation = -1;
        }
    }

    return orientation;
}

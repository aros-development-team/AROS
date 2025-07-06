/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

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

    // If mode == 0, just return current orientation
    if (mode == 0) {
        if (stream->flags & __STDCIO_STDIO_WIDE)
            return 1;
        else
            return -1;
    }

    // If stream is already oriented, return existing orientation
    if (stream->flags & __STDCIO_STDIO_WIDE) {
        if (mode > 0)
            return 1;
        else
            return -1;
    } else {
        // Set orientation based on mode sign
        if (mode > 0)
            stream->flags |= __STDCIO_STDIO_WIDE;
        else
            stream->flags &= ~__STDCIO_STDIO_WIDE;
        return mode > 0 ? 1 : -1;
    }
}

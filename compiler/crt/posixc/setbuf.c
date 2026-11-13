/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    C99 function setbuf().
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

        void setbuf (

/*  SYNOPSIS */
        FILE *stream,
        char *buf)

/*  FUNCTION
        Sets the buffer for the specified stream.

        If `buf` is not NULL, the stream is fully buffered using the provided
        buffer of size `BUFSIZ`. If `buf` is NULL, the stream is set to
        unbuffered mode (no buffering).

        This function is a simpler interface to `setvbuf()` and behaves
        according to the C standard library specification.

    INPUTS
        stream - Pointer to a `FILE` object identifying the stream.
        buf    - Pointer to a buffer for stream buffering, or NULL for
                 unbuffered operation.

    RESULT
        This function returns no value.

    NOTES
        - The size of the buffer used when `buf` is not NULL is fixed to
          `BUFSIZ`.
        - Use `setbuf()` before any I/O operation on the stream to ensure
          the buffer is set properly.
        - This function is equivalent to calling:
          `setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);`

    EXAMPLE
        char buffer[BUFSIZ];
        setbuf(stdout, buffer); // Use a custom buffer for stdout

    BUGS
        - Buffering behavior is implementation-dependent.
        - Calling `setbuf()` after I/O has started on the stream may have
          undefined effects.

    SEE ALSO
        setvbuf(), fflush(), fread(), fwrite()

    INTERNALS
        This function simply calls `setvbuf()` with appropriate mode and size.

******************************************************************************/
{
    setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
} /* setbuf */


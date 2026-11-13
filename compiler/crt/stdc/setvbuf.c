/*
    Copyright (C) 1995-2012, The AROS Development Team. All rights reserved.

    C99 function setvbuf().
*/

#include <dos/stdio.h>
#include <proto/dos.h>
#include <errno.h>

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        int setvbuf (

/*  SYNOPSIS */
        FILE *stream,
        char *buf,
        int mode,
        size_t size)

/*  FUNCTION
        Sets the buffer and the mode associated with a stream.

    INPUTS
        stream: stream to set buffer on
        buf: the buffer to be associated, when NULL a buffer will be allocated
             and freed on close or new setvbuf. Should be longword aligned.
        mode: mode for buffering
            - _IOFBF: fully buffered
            - _IOLBF: line buffered
            - _IONBF: Not buffered
        size: size of the buffer (needs to be at least 208).

    RESULT
        0 on success, EOF on error. errno indicated error.
        Function fails when size < 208 and buf != NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    int bufsize = (int)size;
    char *filebuf = NULL;
    int retval;

    if (!stream)
    {
        errno = EINVAL;
        return EOF;
    }

    /* Pointer has to be longword aligned */
    if (((filebuf = buf) != NULL)
        && (((IPTR)filebuf & 3) != 0))
    {
        char *buf2 = (char *)(((IPTR)filebuf | 3) + 1);
        bufsize -= buf2 - filebuf;
        filebuf = buf2;
    }
    /* Size >= 208 */
    if (bufsize < 208)
    {
        if (filebuf == NULL)
            bufsize = 208;
        else
        {
            errno = EINVAL;
            return EOF;
        }
    }

    switch (mode)
    {
    case _IOFBF:
        mode = BUF_FULL;
        break;
    case _IOLBF:
        mode = BUF_LINE;
        break;
    case _IONBF:
        mode = BUF_NONE;
        break;
    default:
        errno = EINVAL;
        return EOF;
    }

    retval = (int)SetVBuf(stream->fh, filebuf, mode, bufsize ? bufsize : -1);

    return retval;
} /* setvbuf */

/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function fputwc().
*/

#include <wchar.h>
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include "__stdio.h"

#include <limits.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

        wint_t fputwc (

/*  SYNOPSIS */
        wint_t wc,
        FILE * stream)

/*  FUNCTION
        Write a wide character to the specified stream.
        Converts the wide character to UTF-8 and writes the byte sequence.

    INPUTS
        wc - Wide character to write.
        stream - Pointer to the output stream.

    RESULT
        Returns the wide character written, or WEOF on error.

    NOTES
        Handles conversion from wchar_t (Unicode code point) to UTF-8.

    EXAMPLE

    BUGS

    SEE ALSO
        fputws(), putwc()

    INTERNALS

******************************************************************************/
{
    char mb[MB_CUR_MAX];
    int len;

    if (!(stream->flags & __STDCIO_STDIO_WRITE))
    {
        SetIoErr(ERROR_WRITE_PROTECTED);
        errno = EACCES;
        stream->flags |= __STDCIO_STDIO_ERROR;
        return WEOF;
    }

    if ((stream->flags & __STDCIO_STDIO_APPEND))
        Seek(stream->fh, 0, OFFSET_END);

    len = wcrtomb(mb, wc, &stream->mbs);
    if (len < 0)
    {
        errno = EILSEQ;
        stream->flags |= __STDCIO_STDIO_ERROR;
        return WEOF;
    }

    for (int i = 0; i < len; ++i)
    {
        if (FPutC(stream->fh, (UBYTE)mb[i]) == EOF)
        {
            errno = __stdc_ioerr2errno(IoErr());
            stream->flags |= __STDCIO_STDIO_ERROR;
            return WEOF;
        }
    }

    return wc;
}

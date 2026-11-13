/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.

    C99 function freopen().
*/
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdlib.h>

#include "__stdio.h"
#include "__stdcio_intbase.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        FILE *freopen (

/*  SYNOPSIS */
        const char * restrict path,
        const char * restrict mode,
        FILE       * restrict stream
        )

/*  FUNCTION
        Opens the  file whose name is the string pointed to by path  and
        associates  the  stream  pointed to by stream with it.

    INPUTS
        path   - the file to open, NULL to only change the mode of the stream.
        mode   - Mode to open file, see fopen for description of the string.
                 When path is NULL end-of-file and error indicator will be
                 cleared and indication if stream is read and/or write.
                 No change to position in file or no truncation will be
                 performed.
        stream - the stream to which the file will be associated.

    RESULT
        NULL on error or stream. When NULL is returned input stream is
        not changed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
         fopen(), fclose()

    INTERNALS

******************************************************************************/
{
    if (!stream)
        return NULL;

    if (path != NULL)
    {
        struct StdCIOIntBase *StdCIOBase =
            (struct StdCIOIntBase *)__aros_getbase_StdCIOBase();
        FILE *stream2;
        struct MinNode savednode;
        int keepflags;

        /* First close the currently associated file (as if by fclose());
           per C99 any failure to close is ignored. */
        if (!(stream->flags & __STDCIO_STDIO_DONTCLOSE))
            Close(stream->fh);

        stream2 = fopen(path, mode);
        if (!stream2)
        {
            /* The open failed. The old file is already closed, so leave the
               stream in a defined, unusable state and report failure. */
            stream->fh = (BPTR)NULL;
            stream->flags = __STDCIO_STDIO_ERROR;
            return NULL;
        }

        /* Transfer the freshly opened state onto the caller's stream, but
           preserve the caller stream's own list node and its DONTFREE/
           DONTCLOSE attributes (e.g. when reopening stdin/stdout/stderr). */
        savednode = stream->node;
        keepflags = stream->flags & (__STDCIO_STDIO_DONTFREE | __STDCIO_STDIO_DONTCLOSE);

        *stream = *stream2;
        stream->node = savednode;
        stream->flags |= keepflags;

        /* fopen() linked stream2 into the stdcio file list and allocated it
           from the stream pool; unlink and release it (its open handle now
           belongs to stream). */
        Remove((struct Node *)stream2);
        FreePooled(StdCIOBase->streampool, stream2, sizeof(FILE));
    }
    else
    {
        char l2, hasplus;

        l2 = mode[1];
        if (l2 == 'b') l2 = mode[2];
        hasplus = (l2 == '+');

        /* Clear EOF/error indicators and the previous access-mode bits,
           keeping only the stream's lifetime attributes. */
        stream->flags &= ~(__STDCIO_STDIO_TMP | __STDCIO_STDIO_RDWR |
            __STDCIO_STDIO_EOF | __STDCIO_STDIO_ERROR);

        /* Set mode */
        if (hasplus)
            stream->flags |= __STDCIO_STDIO_RDWR;
        else
            switch(mode[0])
            {
            case 'r':
                stream->flags |= __STDCIO_STDIO_READ;
                break;
            case 'w':
            case 'a':
                stream->flags |= __STDCIO_STDIO_WRITE;
                break;
            default:
                return NULL;
            }
    }
    
    return stream;
}

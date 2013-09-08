/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function freopen().
*/
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdlib.h>

#include "__stdio.h"

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
        stream - the stream to wich the file will be associated.

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
        if (!(stream->flags & __STDCIO_STDIO_DONTCLOSE))
            Close(stream->fh);
        FILE *stream2 = fopen(path, mode);
        int dontfree = stream->flags & __STDCIO_STDIO_DONTFREE;
        *stream = *stream2;
        stream->flags |= dontfree;
        Remove((struct Node *)stream2);
        free(stream2);
    }
    else
    {
        char l2, hasplus;

        l2 = mode[1];
        if (l2 == 'b') l2 = mode[2];
        hasplus = (l2 == '+');

        /* Keep some flags; clear the rest */
        stream->flags &= ~(__STDCIO_STDIO_TMP | __STDCIO_STDIO_DONTCLOSE | __STDCIO_STDIO_DONTFREE);

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

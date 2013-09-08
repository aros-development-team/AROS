/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fclose().
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/libcall.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "__stdcio_intbase.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fclose (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION
	Closes a stream.

    INPUTS
	stream - Stream to close.

    RESULT
	Upon successful completion 0 is returned. Otherwise, EOF is
	returned and the global variable errno is set to indicate the
	error. In either case no further access to the stream is possible.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fopen()

    INTERNALS

******************************************************************************/
{
    struct StdCIOIntBase *StdCIOBase =
        (struct StdCIOIntBase *)__aros_getbase_StdCIOBase();

    int ret = 0;
    char s[L_tmpnam+20] = "";

    if (stream->flags & __STDCIO_STDIO_TMP)
    {
        if (!NameFromFH(stream->fh, s, L_tmpnam+20))
        {
            /* Just leave the file in T: */
            D(bug("[fclose]: Could not get name from fh, IoErr()=%d\n", IoErr()));
            s[0] = 0;
        }
    }

    if (!(stream->flags & __STDCIO_STDIO_DONTCLOSE))
    {
        if (Close(stream->fh))
            ret = 0;
        else
        {
            ret = EOF;
            errno = __stdc_ioerr2errno(IoErr());
        }
    }

    Remove((struct Node *)stream);
    if (!(stream->flags & __STDCIO_STDIO_DONTFREE))
        FreePooled(StdCIOBase->streampool, stream, sizeof(FILE));

    if (strlen(s) > 0)
    {
        D(bug("[fclose]: Deleting file '%s'\n", s));
        DeleteFile(s); /* File will be left there if delete fails */
    }

    return ret;
} /* fclose */


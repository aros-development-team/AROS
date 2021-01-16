/*
    Copyright © 1995-2021, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fclose().
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/libcall.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "__stdcio_intbase.h"

#include "debug.h"

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

    D(bug("[%s] %s(0x%p)\n", STDCNAME, __func__, stream));

    if (stream->flags & __STDCIO_STDIO_TMP)
    {
        if (!NameFromFH(stream->fh, s, L_tmpnam+20))
        {
            /* Just leave the file in T: */
            D(bug("[%s] %s: Could not get name from fh, IoErr()=%d\n", STDCNAME, __func__, IoErr()));
            s[0] = 0;
        }
    }

    if (!(stream->flags & __STDCIO_STDIO_DONTCLOSE))
    {
        BOOL closed = Close(stream->fh);
        if (closed)
        {
            D(bug("[%s] %s: closed succesfully\n", STDCNAME, __func__));
            ret = 0;
        }
        else
        {
            LONG ioErr = IoErr();
            D(bug("[%s] %s: Failed to close! (IoErr %x)\n", STDCNAME, __func__, ioErr));

            ret = EOF;
            errno = __stdc_ioerr2errno(ioErr);
        }
    }
    else
    {
        D(bug("[%s] %s: DONTCLOSE set\n", STDCNAME, __func__));
    }

    Remove((struct Node *)stream);
    if (!(stream->flags & __STDCIO_STDIO_DONTFREE))
        FreePooled(StdCIOBase->streampool, stream, sizeof(FILE));

    if (strlen(s) > 0)
    {
        D(bug("[%s] %s: Deleting file '%s'\n", STDCNAME, __func__, s));
        DeleteFile(s); /* File will be left there if delete fails */
    }

    return ret;
} /* fclose */


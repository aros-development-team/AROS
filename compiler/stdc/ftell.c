/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    C99 function ftell()
*/
#include <dos/dos.h>
#include <proto/dos.h>
#include <errno.h>

#include "__stdio.h"

#include <aros/debug.h>

#include "debug.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	long int ftell (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION
	Tell the current position in a stream.

    INPUTS
	stream - Obtain position of this stream

    RESULT
	The position on success and -1 on error.
	If an error occurred, the global variable errno is set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fopen(), fseek(), fwrite()

    INTERNALS

******************************************************************************/
{
    LONG cnt;
    BPTR fh = stream->fh;

    D(bug("[%s] %s: Entering\n", STDCNAME, __func__));

    Flush (fh);
    cnt = Seek (fh, 0, OFFSET_CURRENT);

    if (cnt == -1)
        errno = __stdc_ioerr2errno (IoErr ());

    D(bug("[%s] %s: Leaving cnt=%d\n", STDCNAME, __func__, cnt));

    return (long int)cnt;
} /* ftell */

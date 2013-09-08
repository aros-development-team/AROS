/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fseek()
*/
#include <dos/dos.h>
#include <proto/dos.h>
#include <errno.h>

#include "__stdio.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fseek (

/*  SYNOPSIS */
	FILE * stream,
	long   offset,
	int    whence)

/*  FUNCTION
	Change the current position in a stream.

    INPUTS
	stream - Modify this stream
	offset, whence - How to modify the current position. whence
		can be SEEK_SET, then offset is the absolute position
		in the file (0 is the first byte), SEEK_CUR then the
		position will change by offset (ie. -5 means to move
		5 bytes to the beginning of the file) or SEEK_END.
		SEEK_END means that the offset is relative to the
		end of the file (-1 is the last byte and 0 is
		the EOF).

    RESULT
	0 on success and -1 on error. If an error occurred, the global
	variable errno is set.

    NOTES
        The seek is handled by the files system so effects of what happens
        when seeking after end of file may differ between file systems.

    EXAMPLE

    BUGS
        Not fully compatible with iso fseek, especially in 'ab' and 'a+b'
        modes

    SEE ALSO
	fopen(), fwrite()

    INTERNALS

******************************************************************************/
{
    LONG mode;
    BPTR fh = stream->fh;

    D(bug("[stdcio/fseek()] Entering stream=0x%x, offset=%d, whence=%d\n",
          stream, offset, whence
    ));

    switch (whence)
    {
    case SEEK_SET:
        mode = OFFSET_BEGINNING;
        break;
        
    case SEEK_CUR:
        mode = OFFSET_CURRENT;
        break;
        
    case SEEK_END:
        mode = OFFSET_END;
        break;

    default:
        errno = EINVAL;
        return -1;
    }

    if (Seek(fh, offset, mode) < 0)
    {
        D(bug("[stdcio/fseek()] Failed (IoErr()=%d)\n", IoErr()));
        if (IoErr() == ERROR_UNKNOWN)
            errno = EINVAL;
        else
            errno = __stdc_ioerr2errno(IoErr());
        return -1;
    }
    else
    {
        D(bug("[stdcio/fseek()] Done\n"));
        return 0;
    }
} /* fseek */

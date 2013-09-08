/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fclose().
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <stdlib.h>
#include <unistd.h>
#include "__stdio.h"

#include "__posixc_intbase.h"

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
	fopen(), open(), close()

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    FILENODE * fn;

    if (close(stream->fd) == -1)
    	return EOF;

    fn = FILE2FILENODE (stream);
    Remove ((struct Node *)fn);

    FreePooled(PosixCBase->internalpool, fn, sizeof(FILENODE));

    return 0;
} /* fclose */


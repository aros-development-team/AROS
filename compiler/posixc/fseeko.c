/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Change the position in a stream.
*/

#include <fcntl.h>
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include "__stdio.h"
#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int posixc_fseeko (

/*  SYNOPSIS */
	FILE * stream,
	off_t  offset,
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
	on 32bit platforms, off_t is a 32bit value, and so the 64bit
        version (fseeko64) is needed to work with large files.
        off_t is 64bit natively on 64bit platforms.

    EXAMPLE

    BUGS
	Not fully compatible with iso fseeko, especially in 'ab' and 'a+b'
	modes

    SEE ALSO
	fseek()

    INTERNALS

******************************************************************************/
{

    return __fseeko (stream, offset, whence);

} /* posixc_fseeko */

/*
    Copyright 2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function ftruncate()
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <fcntl.h>
#include <errno.h>
#include "__errno.h"
#include "__open.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	int ftruncate (

/*  SYNOPSIS */
	int   fd,
	off_t length)

/*  FUNCTION
	Truncate a file to a specified length

    INPUTS
	fd     - the descriptor of the file being truncated.
	         The file must be open for writing
	lenght - The file will have at most this size

    RESULT
	0 on success or -1 on errorr.

    NOTES
	If the file previously was larger than this size, the extra  data
	is  lost.   If  the  file  previously  was  shorter, it is
	unspecified whether the  file  is  left  unchanged  or  is
	extended.  In  the  latter case the extended part reads as
	zero bytes.


    EXAMPLE

    BUGS

    SEE ALSO
	open(), truncate()

    INTERNALS

    HISTORY
	4.5.2001 falemagn created

******************************************************************************/
{
    ULONG oldpos;
    size_t size;

    fdesc *fdesc = __getfdesc(fd);

    if (!fdesc)
    {
    	errno = EBADF;
	return -1;
    }

    if (!(fdesc->flags & (O_WRONLY|O_RDWR)))
    {
    	errno = EINVAL;
	return -1;
    }

    oldpos = Seek(fdesc->fh, 0, OFFSET_END);
    size   = Seek(fdesc->fh, 0, OFFSET_CURRENT);
    Seek(fdesc->fh, oldpos, OFFSET_BEGINNING);

    if ((length = SetFileSize(fdesc->fh, length, OFFSET_BEGINNING)) == -1)
    {
    	errno = IoErr2errno(IoErr());
    	return -1;
    }
    else
    if (size < length)
    {
	char buf[16]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	length -= size;

	oldpos = Seek(fdesc->fh, size, OFFSET_BEGINNING);

	while (length >= 16)
	{
	    FWrite(fdesc->fh, buf, 16, 1);
	    length -= 16;
 	}
	if (length)
	    FWrite(fdesc->fh, buf, length, 1);

    	Flush(fdesc->fh);
	Seek(fdesc->fh, oldpos, OFFSET_BEGINNING);
    }

    return 0;
}

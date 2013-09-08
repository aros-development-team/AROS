/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function ftruncate().
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <fcntl.h>
#include <errno.h>
#include "__fdesc.h"

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

    if(fdesc->fcb->privflags & _FCB_ISDIR)
    {
	errno = EISDIR;
	return -1;
    }

    if (!(fdesc->fcb->flags & (O_WRONLY|O_RDWR)))
    {
    	errno = EINVAL;
	return -1;
    }

    oldpos = Seek(fdesc->fcb->fh, 0, OFFSET_END);
    size   = Seek(fdesc->fcb->fh, 0, OFFSET_CURRENT);
    Seek(fdesc->fcb->fh, oldpos, OFFSET_BEGINNING);

    if ((length = SetFileSize(fdesc->fcb->fh, length, OFFSET_BEGINNING)) == -1)
    {
        errno = __stdc_ioerr2errno(IoErr());
    	return -1;
    }
    else
    if (size < length)
    {
	char buf[16]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	length -= size;

	oldpos = Seek(fdesc->fcb->fh, size, OFFSET_BEGINNING);

	while (length >= 16)
	{
	    FWrite(fdesc->fcb->fh, buf, 16, 1);
	    length -= 16;
 	}
	if (length)
	    FWrite(fdesc->fcb->fh, buf, length, 1);

    	Flush(fdesc->fcb->fh);
	Seek(fdesc->fcb->fh, oldpos, OFFSET_BEGINNING);
    }

    return 0;
}

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Reposition read/write file offset.
*/
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <clib/macros.h>
#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	off_t lseek (

/*  SYNOPSIS */
	int    filedes,
	off_t  offset,
	int    whence)

/*  FUNCTION
	Reposition read/write file offset

    INPUTS
	filedef - the filedescriptor being modified
	offset, whence -
	          How to modify the current position. whence
	  	  can be SEEK_SET, then offset is the absolute position
		  in the file (0 is the first byte), SEEK_CUR then the
		  position will change by offset (ie. -5 means to move
		  5 bytes to the beginning of the file) or SEEK_END.
		  SEEK_END means that the offset is relative to the
		  end of the file (-1 is the last byte and 0 is
		  the EOF).

    RESULT
	The new position on success and -1 on error. If an error occurred, the global
	variable errno is set.

    NOTES

    EXAMPLE

    BUGS
	File is extended with zeros if desired position is beyond the end of 
	file.

	Since it's not possible to use Seek() for directories, this 
	implementation fails with EISDIR for directory file descriptors.

    SEE ALSO
	fopen(), fwrite()

    INTERNALS

******************************************************************************/
{
    int  cnt;
    fdesc *fdesc = __getfdesc(filedes);

    if (!fdesc)
    {
	errno = EBADF;
	return -1;
    }

    if(fdesc->fcb->isdir)
    {
	errno = EISDIR;
	return -1;
    }

    switch (whence)
    {
    	case SEEK_SET: whence = OFFSET_BEGINNING; break;
    	case SEEK_CUR: whence = OFFSET_CURRENT; break;
    	case SEEK_END: whence = OFFSET_END; break;

	default:
	    errno = EINVAL;
	    return -1;
    }

    cnt = Seek ((BPTR)fdesc->fcb->fh, offset, whence);

    if (cnt == -1)
    {
	if(IoErr() == ERROR_SEEK_ERROR)
	{
	    LONG saved_error = IoErr();
	    /* Most likely we tried to seek behind EOF. POSIX lseek allows
	       that, and if anything is written at the end on the gap, 
	       reads from the gap should return 0 unless some real data
	       is written there. Since implementing it would be rather
	       difficult, we simply extend the file by writing zeros
	       and hope for the best. */
	    LONG abs_cur_pos = Seek(fdesc->fcb->fh, 0, OFFSET_CURRENT);
	    if(abs_cur_pos == -1)
	        goto error;
	    LONG file_size = Seek(fdesc->fcb->fh, 0, OFFSET_END);
	    if(file_size == -1)
		goto error;
	    /* Now compute how much we have to extend the file */
	    LONG abs_new_pos = 0;
	    switch(whence)
	    {
	        case OFFSET_BEGINNING: abs_new_pos = offset; break;
	        case OFFSET_CURRENT: abs_new_pos = abs_cur_pos + offset; break;
	        case OFFSET_END: abs_new_pos = file_size + offset; break;
	    }
	    if(abs_new_pos > abs_cur_pos)
	    {
		ULONG bufsize = 4096;
		APTR zeros = AllocMem(bufsize, MEMF_ANY | MEMF_CLEAR);
		if(!zeros)
		{
		    /* Restore previous position */
		    Seek(fdesc->fcb->fh, abs_cur_pos, OFFSET_BEGINNING);
		    errno = ENOMEM;
		    return -1;
		}
		
		LONG towrite = abs_new_pos - abs_cur_pos;
		do
		{
		    Write(fdesc->fcb->fh, zeros, MIN(towrite, bufsize));
		    towrite -= bufsize;
		}
		while(towrite > 0);
		
		FreeMem(zeros, bufsize);
	    }
	    else
	    {
		/* Hmm, that's strange. Looks like ERROR_SEEK_ERROR has
		   been caused by something else */
		SetIoErr(saved_error);
		goto error;
	    }
	}
	else
	    goto error;
    }

    return Seek((BPTR)fdesc->fcb->fh, 0, OFFSET_CURRENT);
error:
    errno = __arosc_ioerr2errno (IoErr ());
    return (off_t) -1;
} /* lseek */

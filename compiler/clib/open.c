/*
    Copyright 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function open()
    Lang: english
*/
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <errno.h>
#include "__errno.h"
#include "__open.h"

/*****************************************************************************

    NAME */
#include <fcntl.h>

	int open (

/*  SYNOPSIS */
	const char * pathname,
	int	     flags,
	...)

/*  FUNCTION
	Opens a file with the specified flags and name.

    INPUTS
	pathname - Path and filename of the file you want to open.
	flags - Most be exactly one of: O_RDONLY, O_WRONLY or O_RDWR
		to open a file for reading, writing or for reading and
		writing.

		The mode can be modified by or'ing the following bits in:

		O_CREAT: Create the file if it doesn't exist (only for
			O_WRONLY or O_RDWR). If this flag is set, then
			open() will look for a third parameter mode. mode
			must contain the access modes for the file
			(mostly 0644).
		O_EXCL: Only with O_CREAT. If the file does already exist,
			then open() fails. See BUGS.
		O_NOCTTY:
		O_TRUNC: If the file exists, then it gets overwritten. This
			is the default and the opposite to O_APPEND.
		O_APPEND: If the file exists, then the startung position for
			writes is the end of the file.
		O_NONBLOCK or O_NDELAY: Opens the file in non-blocking mode.
			If there is no data in the file, then read() on a
			terminal will return immediately instead of waiting
			until data arrives. Has no effect on write().
		O_SYNC: The process will be stopped for each write() and the
			data will be flushed before the write() returns.
			This ensures that the data is physically written
			when write() returns. If this flag is not specified,
			the data is written into a buffer and flushed only
			once in a while.

    RESULT
	-1 for error or a file descriptor for use with read(), write(), etc.

    NOTES
	If the filesystem doesn't allow to specify different access modes
	for users, groups and others, then the user modes are used.

        This function must not be used in a shared library or
        in a threaded application.


    EXAMPLE

    BUGS
	The flag O_EXCL is not very reliable if the file resides on a NFS
	filesystem.

	Most flags are not supported right now.

    SEE ALSO
	close(), read(), write(), fopen()

    INTERNALS

    HISTORY
	10.01.97 digulla created

******************************************************************************/
{
    BPTR fh;
    int  openmode;
    int  fd;
    fdesc *currdesc;

    /* filter out invalid modes */
    switch (flags & (O_CREAT|O_TRUNC|O_EXCL))
    {
    	case O_EXCL:
    	case O_EXCL|O_TRUNC:
            errno = EINVAL;
            return -1;
    }

    switch (flags & O_ACCMODE)
    {
        case O_RDONLY:
	    openmode = FMF_READ;
	    break;

        case O_WRONLY:
    	    openmode = FMF_WRITE;
	    break;

    	case O_RDWR:
	    openmode = FMF_WRITE | FMF_READ;
	    break;

     	default:
	    errno = EINVAL;
	    return -1;
    }

    if (flags & O_TRUNC)  openmode |= FMF_CLEAR;
    if (flags & O_CREAT) openmode |= FMF_CREATE;

    currdesc = malloc(sizeof(fdesc));
    if (!currdesc) return -1;

    fd = __getfdslot(__getfirstfd(0));

    if (fd!=-1)
    {
	BPTR fh2 = NULL;

	/* See if the file exists and "lock" it. */
	if (!(flags & O_CREAT) || (flags & O_EXCL))
	{
	    fh2 = Open((char *)pathname, FMF_READ);
	    /* if O_CREAT is not set and the file doesn't exist return an error */
	    if (!fh2 && !(flags & O_CREAT))
	    {
	    	errno = ENOENT;
		free(currdesc);
	  	return -1;
	    }
	    /* if O_EXCL is set and the file already exist return an error */
	    else if (fh2 && (flags & O_EXCL))
	    {
	    	errno = EEXIST;
		free(currdesc);
		Close(fh2);
	  	return -1;
            }
	}

	if (!(fh = Open ((char *)pathname, openmode)) )
    	{
	    errno = IoErr2errno (IoErr ());
	    free(currdesc);
	    return -1;
        }

	currdesc->fh        = fh;
	currdesc->flags     = flags;
	currdesc->opencount = 1;

	__setfdesc(fd, currdesc);

	/* Unlock the file */
	if (fh2) Close(fh2);
    }
    else
    {
        free(currdesc);
    }

    return fd;

} /* open */


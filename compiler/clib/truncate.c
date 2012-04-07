/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function truncate().
*/

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	int truncate (

/*  SYNOPSIS */
	const char *path,
	off_t       length)

/*  FUNCTION
	Truncate a file to a specified length

    INPUTS
	path   - the path of the file being truncated
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
	open()

    INTERNALS

******************************************************************************/
{
    int fd, ret = -1;

    if (!path) /* safety check */
    {
    	errno = EFAULT;
	return -1;
    }

    if ((fd = open(path, O_WRONLY)) != -1)
    {
	ret = ftruncate(fd, length);
	close(fd);
    }

    return ret;
}

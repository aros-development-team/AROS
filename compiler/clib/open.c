/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function open().
*/

#include <stdarg.h>
#include "__open.h"

/*****************************************************************************

    NAME */
#include <fcntl.h>

	int open (

/*  SYNOPSIS */
	const char * pathname,
	int	         flags,
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

******************************************************************************/
{
    mode_t mode = 0644;

    if (flags & O_CREAT)
    {
        va_list ap;

        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    
    return __open(__getfirstfd(0), pathname, flags, mode);
} /* open */


/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function getcwd().
*/

#include <errno.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	int readlink(

/*  SYNOPSIS */
        const char *path,
	char       *buf,
	int         bufsiz)

/*  FUNCTION
        Places the contents of a symbolic link in a buffer of given size. No NUL
	char is appended to the buffer.

    INPUTS
        path   - the path to the symbolic link  
	buf    - pointer to thebuffer where to store the symbolic link content
	bufziz - the size of the buffer in bytes
      
    RESULT
        The call returns the count of characters placed in the buffer if it
        succeeds, or a -1 if an error occurs, placing the error code in the
	global variable errno.
*/
{
    #warning TODO: implement readlink
    errno = ENOSYS;
    return -1;
}	


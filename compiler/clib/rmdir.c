/*
    Copyright � 2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function rmdir().
*/

#define remove remove

#include <proto/dos.h>
#include "__errno.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int rmdir(

/*  SYNOPSIS */
	const char * pathname)

/*  FUNCTION
	Deletes an empty directory.

    INPUTS
	pathname - Complete path to the directory.

    RESULT
	0 on success and -1 on error. In case of an error, errno is set.
	
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
    	unlink(), remove()

    INTERNALS

******************************************************************************/
{
    return remove(pathname);
} /* rmdir() */

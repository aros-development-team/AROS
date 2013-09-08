/*
    Copyright © 2003-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function rmdir().
*/

#include <stdio.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

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
    /* FIXME: Shouldn't we check if pathname is actually a directory ? */
    return remove(pathname);
} /* rmdir() */

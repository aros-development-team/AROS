/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function rename().
*/

#include <proto/dos.h>
#include "__errno.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int rename (

/*  SYNOPSIS */
	const char * oldpath,
	const char * newpath)

/*  FUNCTION
	Renames a file or directory.

    INPUTS
	oldpath - Complete path to existing file or directory.
	newpath - Complete path to the new file or directory.

    RESULT
	0 on success and -1 on error. In case of an error, errno is set.
	
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    if (!Rename ((STRPTR)oldpath,(STRPTR)newpath))
    {
	errno = IoErr2errno (IoErr());
	return -1;
    }

    return 0;

} /* rename */


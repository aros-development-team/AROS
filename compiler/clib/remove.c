/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function remove()
    Lang: english
*/

#define remove remove

#include <proto/dos.h>
#include "__errno.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int remove (

/*  SYNOPSIS */
	const char * pathname)

/*  FUNCTION
	Deletes a file or directory.

    INPUTS
	pathname - Complete path to the file or directory.

    RESULT
	0 on success and -1 on error. In case of an error, errno is set.
	
    NOTES
    	Identical to unlink

    EXAMPLE

    BUGS
    	Does not set errno

    SEE ALSO
    	unlink

    INTERNALS

    HISTORY
	04.04.2001 stegerg created

******************************************************************************/
{
    if (!DeleteFile ((STRPTR)pathname))
    {
	errno = IoErr2errno (IoErr());
	return -1;
    }

    return 0;
    
} /* remove */


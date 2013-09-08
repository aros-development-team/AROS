/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function setgid().
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	int setgid(

/*  SYNOPSIS */
	gid_t gid)

/*  FUNCTION
	
    INPUTS
	
    RESULT
	
    NOTES
        Not implemented.

    EXAMPLE

    BUGS
    	
    SEE ALSO
        
    INTERNALS

******************************************************************************/
{
    /* TODO: Implement setgid() */
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
    errno = ENOSYS;

    return 0;
} /* setgid() */

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function setgid().
*/

#include <aros/debug.h>

#include <sys/types.h>
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
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;

    return 0;
} /* setgid() */

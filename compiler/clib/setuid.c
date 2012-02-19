/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function setuid().
*/

#include <aros/debug.h>

#include <sys/types.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	int setuid(

/*  SYNOPSIS */
	uid_t uid)

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
    /* TODO: Implement setuid() */
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;

    return 0;
} /* setuid() */

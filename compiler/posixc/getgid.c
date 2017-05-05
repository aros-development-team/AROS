/*
    Copyright © 2003-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function getgid().
*/

#include "__posixc_intbase.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	gid_t getgid(

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Returns the real group ID of the calling process

    INPUTS
	
    RESULT
	
    NOTES

    EXAMPLE

    BUGS
    	
    SEE ALSO
    	setgid()
        
    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    return (gid_t)PosixCBase->gid;
} /* getgid() */

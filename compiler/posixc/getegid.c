/*
    Copyright © 2003-2017, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function getegid().
*/

#include "__posixc_intbase.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	gid_t getegid(

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Returns the effective group ID of the calling process

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

    return (gid_t)PosixCBase->egid;
} /* getegid() */

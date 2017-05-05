/*
    Copyright © 2003-2016, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function getuid().
*/

#include "__posixc_intbase.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        uid_t getuid(

/*  SYNOPSIS */
        void)

/*  FUNCTION
	Returns the real user ID of the calling process.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        geteuid()
        
    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    return PosixCBase->uid;
} /* getuid() */


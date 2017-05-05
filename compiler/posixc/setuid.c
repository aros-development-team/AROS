/*
    Copyright © 2004-2016, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function setuid().
*/

#include <aros/debug.h>
#include <errno.h>

#include "__posixc_intbase.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        int setuid(

/*  SYNOPSIS */
        uid_t uid)

/*  FUNCTION
	Sets the user ID, and effective user ID of the calling process.

    INPUTS

    RESULT

    NOTES
        Does not check permissions.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    PosixCBase->euid = PosixCBase->uid = uid;

    return 0;
} /* setuid() */

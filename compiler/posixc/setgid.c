/*
    Copyright (C) 2004-2017, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function setgid().
*/

#include <aros/debug.h>
#include <errno.h>

#include "__posixc_intbase.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        int setgid(

/*  SYNOPSIS */
        gid_t gid)

/*  FUNCTION
        Set the group id, and effective group id of the calling process to gid.

    INPUTS
        
    RESULT
        
    NOTES

    EXAMPLE

    BUGS
        
    SEE ALSO
        
    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    PosixCBase->egid = PosixCBase->gid = gid;

    return 0;
} /* setgid() */

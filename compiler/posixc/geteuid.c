/*
    Copyright (C) 2003-2016, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function geteuid().
*/

#include "__posixc_intbase.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        uid_t geteuid(

/*  SYNOPSIS */
        void)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        getuid()
        
    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    return PosixCBase->euid;
} /* geteuid() */

/*
    Copyright (C) 2016, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function seteuid().
*/

#include <aros/debug.h>
#include <errno.h>

#include "__posixc_intbase.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        int seteuid(

/*  SYNOPSIS */
        uid_t uid)

/*  FUNCTION

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

    PosixCBase->euid = uid;

    return 0;
} /* seteuid() */

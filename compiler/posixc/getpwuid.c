/*
    Copyright © 2004-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include "__posixc_intbase.h"
#include "__usergrp.h"

/*****************************************************************************

    NAME */

#include <pwd.h>

        struct passwd *getpwuid(

/*  SYNOPSIS */
        uid_t uid)

/*  FUNCTION
	Returns the database entry for the user with specified uid.

    INPUTS

    RESULT

    NOTES
	Function is not re-entrant. Results will be overwritten by
	subsequent calls.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    __fill_passwd(&PosixCBase->pwd, uid);

    if (PosixCBase->pwd.pw_uid == uid)
        return &PosixCBase->pwd;

    return NULL;
}


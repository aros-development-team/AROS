/*
    Copyright © 2004-2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <errno.h>

#include "__usergrp.h"

/*****************************************************************************

    NAME */

#include <pwd.h>

        struct passwd *getpwuid(

/*  SYNOPSIS */
        uid_t uid)

/*  FUNCTION

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
    static struct passwd _return;

    if (_user.ur_uid == uid)
    {
        _return.pw_name     = _user.ur_name;
        _return.pw_uid      = _user.ur_uid;
        _return.pw_dir      = _user.ur_dir;
        _return.pw_shell    = _user.ur_shell;
        _return.pw_passwd   = _user.ur_passwd;
        _return.pw_gecos    = _user.ur_gecos;

        return &_return;
    }

    return NULL;
}


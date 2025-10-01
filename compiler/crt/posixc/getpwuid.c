/*
    Copyright (C) 2004-2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#define __NOBLIBBASE__

#include "__posixc_intbase.h"
#include "__optionallibs.h"

#include "__usergrp.h"

/*****************************************************************************

    NAME */

#include <pwd.h>

        struct passwd *getpwuid(

/*  SYNOPSIS */
        uid_t uid)

/*  FUNCTION
        Operates on the user database via netinfo.device
        interface, Providing a convenient unix-compatible interface to
        the password unit of the netinfo.device.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        usergroup.library/getpwuid()

    INTERNALS
        Implementation handled by usergroup.library, if available.

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    if (__usergroup_available(PosixCBase)) {
        return AROS_LC1(struct passwd *, getpwuid,
             AROS_LCA(uid_t, uid, D0),
            struct Library *, PosixCBase->PosixCUserGroupBase, 20, Usergroup);
    } else {
        __fill_passwd(&PosixCBase->pwd, uid);

        if (PosixCBase->pwd.pw_uid == uid)
            return &PosixCBase->pwd;
    }
    return NULL;
}

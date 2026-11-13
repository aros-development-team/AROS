/*
    Copyright (C) 2004-2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <errno.h>

#define __NOBLIBBASE__

#include "__posixc_intbase.h"
#include "__optionallibs.h"

/*****************************************************************************

    NAME */

#include <pwd.h>

        struct passwd *getpwent(

/*  SYNOPSIS */
        void)

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
        usergroup.library/getpwent()

    INTERNALS
        Implementation handled by usergroup.library, if available.

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    if (__usergroup_available(PosixCBase)) {
        return AROS_LC0(struct passwd *, getpwent,
            struct Library *, PosixCBase->PosixCUserGroupBase, 22, Usergroup);
    }
    errno = ENOSYS;
    return NULL;
}


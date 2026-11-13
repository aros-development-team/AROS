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

        void endpwent(

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
        usergroup.library/endpwent()

    INTERNALS
        Implementation handled by usergroup.library, if available.

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    if (__usergroup_available(PosixCBase)) {
        AROS_LC0NR(void, endpwent,
            struct Library *, PosixCBase->PosixCUserGroupBase, 23, Usergroup);
    } else
        errno = ENOSYS;
    return;
}

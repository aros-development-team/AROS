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

#include <grp.h>

        struct group *getgrgid(

/*  SYNOPSIS */
        gid_t gid)

/*  FUNCTION
        Operates on the group database via netinfo.device
        interface, Providing a convenient unix-compatible interface to
        the group unit of the netinfo.device.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        usergroup.library/getgrgid()

    INTERNALS
        Implementation handled by usergroup.library, if available.

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    if (__usergroup_available(PosixCBase)) {
        return AROS_LC1(struct group *, getgrgid,
             AROS_LCA(gid_t, gid, D0),
            struct Library *, PosixCBase->PosixCUserGroupBase, 25, Usergroup    );
    }
    errno = ENOSYS;
    return NULL;
}


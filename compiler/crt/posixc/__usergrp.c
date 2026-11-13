/*
    Copyright (C) 2016-2017, The AROS Development Team. All rights reserved.

    Desc: single user defintion for POSIX user/groups
*/

#include LC_LIBDEFS_FILE
#include "__posixc_intbase.h"

#include <exec/types.h>
#include <aros/symbolsets.h>

#include "__usergrp.h"

#define DEF_USER_UID    1000
#define DEF_USER_GID    1000
#define DEF_USER_NAME   "aros"
#define DEF_USER_SHELL  "sh"
#define DEF_USER_DIR    "/home"

void __fill_passwd(struct passwd *pwd, uid_t uid)
{
    /* we only support the hard coded default user atm */
    pwd->pw_name     = DEF_USER_NAME;
    pwd->pw_uid      = DEF_USER_UID;
    pwd->pw_gid      = DEF_USER_GID;
    pwd->pw_dir      = DEF_USER_DIR;
    pwd->pw_shell    = DEF_USER_SHELL;
    pwd->pw_passwd   = DEF_USER_NAME;
    pwd->pw_gecos    = DEF_USER_NAME;
}

int __init_usergrp(struct PosixCIntBase *PosixCIntBase)
{
    struct PosixCIntBase *parent = __GM_GetBaseParent(PosixCIntBase);

    /* Match umask inheritance: a new program CRT must retain all four IDs
     * across vfork/exec instead of resetting them to the default user. */
    if (parent && (parent->flags & (VFORK_PARENT | EXEC_PARENT)))
    {
        PosixCIntBase->uid  = parent->uid;
        PosixCIntBase->euid = parent->euid;
        PosixCIntBase->gid  = parent->gid;
        PosixCIntBase->egid = parent->egid;
    }
    else
    {
        PosixCIntBase->uid  = DEF_USER_UID;
        PosixCIntBase->euid = DEF_USER_UID;
        PosixCIntBase->gid  = DEF_USER_GID;
        PosixCIntBase->egid = DEF_USER_GID;
    }

    return 1;
}

ADD2OPENLIB(__init_usergrp, 5);

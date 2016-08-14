/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
    $Id$

    Desc: single user defintion for POSIX user/groups
    Lang: English
*/

#include "__posixc_intbase.h"

#include <exec/types.h>
#include <aros/symbolsets.h>

#include "__usergrp.h"

#define USER_UID    1000
#define USER_GID    1000
#define USER_NAME   "aros"
#define USER_SHELL  "sh"
#define USER_DIR    "/home"

void __fill_passwd(struct passwd *pwd)
{
    pwd->pw_name     = USER_NAME;
    pwd->pw_uid      = USER_UID;
    pwd->pw_gid      = USER_GID;
    pwd->pw_dir      = USER_DIR;
    pwd->pw_shell    = USER_SHELL;
    pwd->pw_passwd   = USER_NAME;
    pwd->pw_gecos    = USER_NAME;
}

int __init_usergrp(struct PosixCIntBase *PosixCIntBase)
{
    PosixCIntBase->uid      = USER_UID;
    PosixCIntBase->euid     = USER_UID;

    return 1;
}

ADD2OPENLIB(__init_usergrp, 5);

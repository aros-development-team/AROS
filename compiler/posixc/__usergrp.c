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

#define FIRST_UID   1000
#define FIRST_GID   1000

struct userrecord _user =
{
    "aros",
    FIRST_UID,
    FIRST_GID,
    "/home",
    "sh",
    "aros",
    "AROS"
};

int __init_usergrp(struct PosixCIntBase *PosixCIntBase)
{
    PosixCIntBase->uid      = _user.ur_uid;
    PosixCIntBase->euid     = _user.ur_uid;

    return 1;
}

ADD2OPENLIB(__init_usergrp, 5);

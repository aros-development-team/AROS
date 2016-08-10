#ifndef ___USERGRP_H
#define ___USERGRP_H

/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/gid_t.h>
#include <aros/types/uid_t.h>

struct userrecord
{
    char  *ur_name;    /* Username */
    uid_t  ur_uid;     /* Read User ID */
    gid_t  ur_gid;     /* Group ID */
    char  *ur_dir;     /* Home directory  */
    char  *ur_shell;   /* Shell */
    char  *ur_passwd;  /* Password */
    char  *ur_gecos;   /* Real name */
};

extern struct userrecord _user;

#endif /* ___USERGRP_H */

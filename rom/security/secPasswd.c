/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_server.h"
#include "security_segment.h"
#include "security_monitor.h"
#include "security_memory.h"
#include "security_plugins.h"
#include "security_crypto.h"
#include "security_enforce.h"
#include "security_packetio.h"
#include "security_userinfo.h"
#include "security_groupinfo.h"
#include "security_login.h"
#include "security_support.h"

/*****************************************************************************

    NAME */
        AROS_LH2(BOOL, secPasswd,

/*  SYNOPSIS */
        AROS_LHA(STRPTR, oldpwd, A0),
        AROS_LHA(STRPTR, newpwd, A1),

/*  LOCATION */
        struct SecurityBase *, secBase, 8, Security)

/*
    FUNCTION
        Change the password of the owner of the calling task.

    INPUTS
        oldpwd - the old password.
        newpwd - the new password.

    RESULT
        success - TRUE if the password was changed.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (BOOL)SendServerPacket(secBase, secSAction_Passwd, (SIPTR)oldpwd, (SIPTR)newpwd, 0, 0);

    AROS_LIBFUNC_EXIT
} /* secPasswd */

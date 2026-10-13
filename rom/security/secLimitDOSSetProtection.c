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
        AROS_LH1(BOOL, secLimitDOSSetProtection,

/*  SYNOPSIS */
        AROS_LHA(BOOL, flag, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 16, Security)

/*
    FUNCTION
        Limit dos.library/SetProtection() to changing only the protection
        bits for the owner of a file; the GROUP and OTHER bits can then only
        be changed via secSetProtection(). Root only.

    INPUTS
        flag - TRUE to turn limiting on.

    RESULT
        success

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!CallerIsRoot(secBase))
        return FALSE;
    secBase->LimitDOSSetProtection = flag ? TRUE : FALSE;
    return TRUE;

    AROS_LIBFUNC_EXIT
} /* secLimitDOSSetProtection */

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
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
        AROS_LH0(BOOL, secIsConfigured,

/*  SYNOPSIS */

/*  LOCATION */
        struct SecurityBase *, secBase, 58, Security)

/*
    FUNCTION
        Is the system configured for multi-user operation, i.e. has a
        password file been found and parsed? On an unconfigured system
        every task is treated as privileged and no access checks are made.

    RESULT
        TRUE if configured and no security violation was detected.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return secBase->Configured && !secBase->SecurityViolation;

    AROS_LIBFUNC_EXIT
} /* secIsConfigured */

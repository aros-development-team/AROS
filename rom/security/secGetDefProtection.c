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
        AROS_LH1(ULONG, secGetDefProtection,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 14, Security)

/*
    FUNCTION
        Get the default protection bits of a task.

    INPUTS
        task - the task, NULL for the current one.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return GetTaskDefProtection(secBase, task ? task : FindTask(NULL));

    AROS_LIBFUNC_EXIT
} /* secGetDefProtection */

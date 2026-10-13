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
        AROS_LH1(BOOL, secAddMonitor,

/*  SYNOPSIS */
        AROS_LHA(struct secMonitor *, monitor, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 28, Security)

/*
    FUNCTION
        Add a monitor: a task interested in login/passwd/owner-change
        events. Root only. The structure must stay valid until
        secRemMonitor().

    RESULT
        success

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!monitor || !CallerIsRoot(secBase))
        return FALSE;
    ObtainSemaphore(&secBase->MonitorSem);
    AddHead((struct List *)&secBase->MonitorList, (struct Node *)&monitor->Node);
    ReleaseSemaphore(&secBase->MonitorSem);
    return TRUE;

    AROS_LIBFUNC_EXIT
} /* secAddMonitor */

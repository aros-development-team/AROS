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
        AROS_LH1(BOOL, secKill,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 30, Security)

/*
    FUNCTION
        Kill a task. Root only. Processes are asked to quit with
        SIGBREAKF_CTRL_C; plain tasks are removed with RemTask().

    RESULT
        success

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!task || task == FindTask(NULL) || task == (struct Task *)secBase->Server || !CallerIsRoot(secBase))
        return FALSE;

    Forbid();
    if (task->tc_Node.ln_Type == NT_PROCESS)
        Signal(task, SIGBREAKF_CTRL_C);
    else
        RemTask(task);
    Permit();
    return TRUE;

    AROS_LIBFUNC_EXIT
} /* secKill */

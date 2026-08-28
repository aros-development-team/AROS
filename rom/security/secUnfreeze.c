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
        AROS_LH1(BOOL, secUnfreeze,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 32, Security)

/*
    FUNCTION
        Unfreeze a task frozen with secFreeze(). Root only.

    RESULT
        success

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    return FALSE;
#else
    struct secTaskNode *node;
    BOOL res = FALSE;

    if (!task || !CallerIsRoot(secBase))
        return FALSE;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindTaskNode(secBase, task)) && (node->Flags & secTNF_Frozen))
    {
        Disable();
        Remove(&task->tc_Node);
        node->Flags &= ~secTNF_Frozen;
        if (node->FrozenState == TS_READY)
            Enqueue(&SysBase->TaskReady, &task->tc_Node);
        else
            Enqueue(&SysBase->TaskWait, &task->tc_Node);
        Enable();
        res = TRUE;
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return res;
#endif

    AROS_LIBFUNC_EXIT
} /* secUnfreeze */

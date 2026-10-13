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
        AROS_LH1(BOOL, secSetDefProtectionA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 13, Security)

/*
    FUNCTION
        Set the default protection bits ('umask') used for new files.

    TAGS
        secT_Task          - (struct Task *) the task, default the current one.
        secT_DefProtection - (ULONG) the protection bits, default
                             FIBF_OTR_READ|FIBF_GRP_READ.
        secT_Global        - (BOOL) also change all descendants of the task.

    RESULT
        success

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secTags tags;
    struct secTaskNode *node;
    BOOL res = FALSE;

    if (!InterpretTagList(secBase, taglist, &tags))
        return FALSE;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindOrCreateTaskNode(secBase, tags.Task)))
    {
        node->DefProtection = tags.DefProtection;
        if (tags.Global)
        {
            struct MinNode *n;
            ForeachNode(&node->Children, n)
                TASKNODE_FROM_SIBLINGS(n)->DefProtection = tags.DefProtection;
        }
        res = TRUE;
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return res;

    AROS_LIBFUNC_EXIT
} /* secSetDefProtectionA */

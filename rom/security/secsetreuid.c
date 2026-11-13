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
        AROS_LH2(int, secsetreuid,

/*  SYNOPSIS */
        AROS_LHA(int, ruid, D0),
        AROS_LHA(int, euid, D1),

/*  LOCATION */
        struct SecurityBase *, secBase, 45, Security)

/*
    FUNCTION
        POSIX: set the real and effective user ids. -1 leaves an id
        unchanged.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secTaskNode *node;
    int rc = -1;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindOrCreateTaskNode(secBase, FindTask(NULL))) && node->Owner)
    {
        BOOL isroot = (node->Owner->uid == secROOT_UID);
        BOOL ok = TRUE;

        if (ruid != -1 && !isroot && ruid != node->RealUID && ruid != node->Owner->uid)
            ok = FALSE;
        if (euid != -1 && !isroot && euid != node->RealUID && euid != node->Owner->uid && euid != node->SavedUID)
            ok = FALSE;
        if (ok)
        {
            if (ruid != -1)
                node->RealUID = ruid;
            if (euid != -1)
                node->Owner->uid = euid;
            if (ruid != -1 || (euid != -1 && euid != node->RealUID))
                node->SavedUID = node->Owner->uid;
            rc = 0;
        }
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return rc;

    AROS_LIBFUNC_EXIT
} /* secsetreuid */

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
        AROS_LH1(int, secsetegid,

/*  SYNOPSIS */
        AROS_LHA(UWORD, gid, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 44, Security)

/*
    FUNCTION
        POSIX: set the effective group id.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secTaskNode *node;
    int rc = -1;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindOrCreateTaskNode(secBase, FindTask(NULL))) && node->Owner)
    {
        if (node->Owner->uid == secROOT_UID || gid == node->RealGID || gid == node->SavedGID)
        {
            node->Owner->gid = gid;
            rc = 0;
        }
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return rc;

    AROS_LIBFUNC_EXIT
} /* secsetegid */

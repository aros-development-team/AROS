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
        AROS_LH1(int, secgetpgid,

/*  SYNOPSIS */
        AROS_LHA(int, pid, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 46, Security)

/*
    FUNCTION
        POSIX: the session id of a process (0 = the calling one).

    RESULT
        The session id, 0 if not in a session, -1 for an unknown pid.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secTaskNode *node;
    int rc = -1;

    ObtainSemaphoreShared(&secBase->TaskOwnerSem);
    node = (pid == 0) ? FindTaskNode(secBase, FindTask(NULL)) : FindTaskNodePid(secBase, pid);
    if (node)
        rc = node->Session ? node->Session->sid : 0;
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return rc;

    AROS_LIBFUNC_EXIT
} /* secgetpgid */

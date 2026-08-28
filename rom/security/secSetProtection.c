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
        AROS_LH2(BOOL, secSetProtection,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(LONG, mask, D2),

/*  LOCATION */
        struct SecurityBase *, secBase, 15, Security)

/*
    FUNCTION
        Change the protection bits of a file or directory, including the
        GROUP and OTHER bits, regardless of secLimitDOSSetProtection().

    INPUTS
        name - the file or directory.
        mask - the new protection bits.

    RESULT
        success

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secTaskNode *node;
    BOOL res;

    if (!secBase->sec_AfterDOSDone)
        return FALSE;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindOrCreateTaskNode(secBase, FindTask(NULL))))
        node->Flags |= secTNF_NoLimit;
    ReleaseSemaphore(&secBase->TaskOwnerSem);

    res = SetProtection(name, mask);

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindTaskNode(secBase, FindTask(NULL))))
        node->Flags &= ~secTNF_NoLimit;
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return res;

    AROS_LIBFUNC_EXIT
} /* secSetProtection */

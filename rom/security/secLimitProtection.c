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
        AROS_LH2(LONG, secLimitProtection,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(LONG, protect, D2),

/*  LOCATION */
        struct SecurityBase *, secBase, 59, Security)

/*
    FUNCTION
        Used by dos.library/SetProtection(): apply the LIMITDOSSETPROTECTION
        policy. If limiting is on and the caller is neither root nor inside
        secSetProtection(), the GROUP and OTHER bits of the existing object
        are preserved.

    INPUTS
        name    - the object SetProtection() is about to change.
        protect - the requested protection bits.

    RESULT
        The protection bits to apply.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secTaskNode *node;
    struct FileInfoBlock *fib;
    BPTR lock;
    BOOL limit;

    if (!secBase->LimitDOSSetProtection || !secBase->Configured || CallerIsRoot(secBase))
        return protect;

    ObtainSemaphoreShared(&secBase->TaskOwnerSem);
    node = FindTaskNode(secBase, FindTask(NULL));
    limit = !(node && (node->Flags & secTNF_NoLimit));
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    if (!limit)
        return protect;

    if ((lock = Lock(name, SHARED_LOCK)))
    {
        if ((fib = AllocDosObject(DOS_FIB, NULL)))
        {
            if (Examine(lock, fib))
            {
                LONG keep = FIBF_GRP_DELETE | FIBF_GRP_EXECUTE | FIBF_GRP_WRITE | FIBF_GRP_READ |
                            FIBF_OTR_DELETE | FIBF_OTR_EXECUTE | FIBF_OTR_WRITE | FIBF_OTR_READ |
                            secFIBF_SET_UID | secFIBF_SET_GID;
                protect = (protect & ~keep) | (fib->fib_Protection & keep);
            }
            FreeDosObject(DOS_FIB, fib);
        }
        UnLock(lock);
    }
    return protect;

    AROS_LIBFUNC_EXIT
} /* secLimitProtection */

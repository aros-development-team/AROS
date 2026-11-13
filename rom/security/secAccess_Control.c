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
        AROS_LH6(LONG, secAccess_Control,

/*  SYNOPSIS */
        AROS_LHA(ULONG, contextflags, D1),
        AROS_LHA(APTR, context, A1),
        AROS_LHA(struct secExtOwner *, task, A2),
        AROS_LHA(ULONG, objectowner, D2),
        AROS_LHA(LONG, objectprot, D3),
        AROS_LHA(LONG, access_type, D4),

/*  LOCATION */
        struct SecurityBase *, secBase, 33, Security)

/*
    FUNCTION
        Filesystem API: decide whether a task may access an object.

    INPUTS
        contextflags - secAC_IGNORE_CONTEXT or secAC_FILESYSTEM_CONTEXT.
        context      - the filesystem's MsgPort for secAC_FILESYSTEM_CONTEXT.
        task         - the requesting task's owner (see secGetPktOwner()),
                       NULL for nobody.
        objectowner  - owner of the object (uid<<16 | gid).
        objectprot   - protection bits of the object.
        access_type  - secAt_#? flags.

    RESULT
        secAC_PERMISSION_GRANTED, or secAC_PERMISSION_DENIED with secAC_#?
        detail flags.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secVolume *vol = NULL;

    if (contextflags == secAC_FILESYSTEM_CONTEXT && context)
    {
        struct secVolume *v;

        ObtainSemaphoreShared(&secBase->VolumesSem);
        for (v = secBase->Volumes; v; v = v->Next)
            if (v->Process == (struct MsgPort *)context || v->OrigProc == (struct MsgPort *)context)
            {
                vol = v;
                break;
            }
        ReleaseSemaphore(&secBase->VolumesSem);
    }
    return IsAllowed(secBase, vol, task, objectowner, objectprot, access_type);

    AROS_LIBFUNC_EXIT
} /* secAccess_Control */

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
        AROS_LH1(struct secExtOwner *, secUserInfo2ExtOwner,

/*  SYNOPSIS */
        AROS_LHA(struct secUserInfo *, info, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 24, Security)

/*
    FUNCTION
        Convert a User Information Structure to an extended owner.

    RESULT
        owner - free with secFreeExtOwner().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secExtOwner *owner;
    ULONG size;

    if (!info)
        return NULL;
    size = sizeof(struct secExtOwner) + info->NumSecGroups * sizeof(UWORD);
    if ((owner = MAlloc(size)))
    {
        owner->uid = info->uid;
        owner->gid = info->gid;
        owner->NumSecGroups = info->NumSecGroups;
        if (info->NumSecGroups)
            CopyMem(info->SecGroups, secSecGroups(owner), info->NumSecGroups * sizeof(UWORD));
    }
    return owner;

    AROS_LIBFUNC_EXIT
} /* secUserInfo2ExtOwner */

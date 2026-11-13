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
        AROS_LH2(struct secUserInfo *, secGetUserInfo,

/*  SYNOPSIS */
        AROS_LHA(struct secUserInfo *, info, A0),
        AROS_LHA(ULONG, keytype, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 11, Security)

/*
    FUNCTION
        Fill in a User Information Structure according to the key. Repeated
        calls with secKeyType_Next (and the #?Next keys) iterate.

    INPUTS
        info    - structure allocated with secAllocUserInfo(), the key field
                  filled in.
        keytype - secKeyType_#?, see <libraries/security.h>.

    RESULT
        info, or NULL if no (more) user matches.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!info)
        return NULL;
    return (struct secUserInfo *)SendServerPacket(secBase, secSAction_GetUserInfo, (SIPTR)info, (SIPTR)keytype, 0, 0);

    AROS_LIBFUNC_EXIT
} /* secGetUserInfo */

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
        AROS_LH2(struct secGroupInfo *, secGetGroupInfo,

/*  SYNOPSIS */
        AROS_LHA(struct secGroupInfo *, info, A0),
        AROS_LHA(ULONG, keytype, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 27, Security)

/*
    FUNCTION
        Fill in a Group Information Structure according to the key.

    RESULT
        info, or NULL if no (more) group matches.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!info)
        return NULL;
    return (struct secGroupInfo *)SendServerPacket(secBase, secSAction_GetGroupInfo, (SIPTR)info, (SIPTR)keytype, 0, 0);

    AROS_LIBFUNC_EXIT
} /* secGetGroupInfo */

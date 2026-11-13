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
        AROS_LH1(void, secFreeGroupInfo,

/*  SYNOPSIS */
        AROS_LHA(struct secGroupInfo *, info, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 26, Security)

/*
    FUNCTION
        Free a structure allocated with secAllocGroupInfo().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secPrivGroupInfo *pinfo = (struct secPrivGroupInfo *)info;

    if (pinfo)
    {
        if (pinfo->Pattern)
            FreeV(pinfo->Pattern);
        Free(pinfo, sizeof(struct secPrivGroupInfo));
    }

    AROS_LIBFUNC_EXIT
} /* secFreeGroupInfo */

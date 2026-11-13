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
        AROS_LH2(void, secPluginOperationComplete,

/*  SYNOPSIS */
        AROS_LHA(APTR, context, A0),
        AROS_LHA(ULONG, result, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 49, Security)

/*
    FUNCTION
        Plugin API: complete an operation that returned secpiASYNC.

    INPUTS
        context - the server packet handed to the plugin.
        result  - the secpi#? result.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secSPacket *pkt = (struct secSPacket *)context;

    if (pkt)
    {
        pkt->AsyncResult = result;
        pkt->Stage++;
        if (secBase->ServerPort)
            PutMsg(secBase->ServerPort, (struct Message *)pkt);
    }

    AROS_LIBFUNC_EXIT
} /* secPluginOperationComplete */

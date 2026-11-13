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
        AROS_LH1(LONG, secGetPktDefProtection,

/*  SYNOPSIS */
        AROS_LHA(struct DosPacket *, pkt, A1),

/*  LOCATION */
        struct SecurityBase *, secBase, 35, Security)

/*
    FUNCTION
        Filesystem API: get the default protection bits of the task that
        sent a packet, to be used for newly created objects.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return pkt ? GetPktDefProtection(secBase, pkt) : secDEFPROTECTION;

    AROS_LIBFUNC_EXIT
} /* secGetPktDefProtection */

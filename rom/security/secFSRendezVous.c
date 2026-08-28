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
        AROS_LH0(BOOL, secFSRendezVous,

/*  SYNOPSIS */

/*  LOCATION */
        struct SecurityBase *, secBase, 18, Security)

/*
    FUNCTION
        Called by a multi-user aware filesystem when it comes up, so that the
        library rescans the volumes and key files.

    RESULT
        TRUE if the server is running.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (secBase->Server && secBase->ServerPort)
    {
        Signal((struct Task *)secBase->Server, 1UL << secBase->ConsistencySig);
        return TRUE;
    }
    return FALSE;

    AROS_LIBFUNC_EXIT
} /* secFSRendezVous */

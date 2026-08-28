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
        AROS_LH0(BPTR, secGetConfigDirLock,

/*  SYNOPSIS */

/*  LOCATION */
        struct SecurityBase *, secBase, 20, Security)

/*
    FUNCTION
        Get a shared lock on the configuration directory.

    RESULT
        lock, or BNULL. UnLock() it when done.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (BPTR)SendServerPacket(secBase, secSAction_ConfigDirLock, 0, 0, 0, 0);

    AROS_LIBFUNC_EXIT
} /* secGetConfigDirLock */

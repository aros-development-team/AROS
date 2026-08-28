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
        AROS_LH1(void, secUnlocksecBase,

/*  SYNOPSIS */
        AROS_LHA(struct secPointers *, secP, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 38, Security)

/*
    FUNCTION
        Release the lists locked with secLocksecBase().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!secP)
        return;
    ReleaseSemaphore(&secBase->VolumesSem);
    ReleaseSemaphore(&secBase->MonitorSem);
    ReleaseSemaphore(&secBase->SegOwnerSem);
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    ReleaseSemaphore(&secBase->SuperSem);
    FreeV(secP);

    AROS_LIBFUNC_EXIT
} /* secUnlocksecBase */

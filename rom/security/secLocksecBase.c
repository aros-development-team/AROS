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
        AROS_LH0(struct secPointers *, secLocksecBase,

/*  SYNOPSIS */

/*  LOCATION */
        struct SecurityBase *, secBase, 37, Security)

/*
    FUNCTION
        Lock the library's internal lists for inspection by privileged
        tools. Root only. Release with secUnlocksecBase() as soon as
        possible.

    RESULT
        pointers to the lists, or NULL.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secPointers *ptr;

    if (!CallerIsRoot(secBase))
        return NULL;

    ObtainSemaphore(&secBase->SuperSem);
    ObtainSemaphoreShared(&secBase->TaskOwnerSem);
    ObtainSemaphoreShared(&secBase->SegOwnerSem);
    ObtainSemaphoreShared(&secBase->MonitorSem);
    ObtainSemaphoreShared(&secBase->VolumesSem);
    if ((ptr = MAllocV(sizeof(struct secPointers))))
    {
        ptr->Monitors = &secBase->MonitorList;
        ptr->Segments = &secBase->SegOwnerList;
        ptr->Sessions = &secBase->SessionsList;
        ptr->Tasks = secBase->TaskOwnerList;
        ptr->Volumes = secBase->Volumes;
        return ptr;
    }
    ReleaseSemaphore(&secBase->VolumesSem);
    ReleaseSemaphore(&secBase->MonitorSem);
    ReleaseSemaphore(&secBase->SegOwnerSem);
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    ReleaseSemaphore(&secBase->SuperSem);
    return NULL;

    AROS_LIBFUNC_EXIT
} /* secLocksecBase */

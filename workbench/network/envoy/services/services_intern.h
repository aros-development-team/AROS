#ifndef SERVICES_INTERN_H
#define SERVICES_INTERN_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: services.library - library base
*/

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <envoy/nipc.h>
#include <envoy/services.h>

#include "services_private.h"

struct ServicesBase
{
    struct Library          sb_Lib;
    struct SignalSemaphore  sb_Sem;         /* guards sb_Services                           */
    struct SignalSemaphore  sb_OpenSem;
    struct MinList          sb_Services;    /* of struct ServiceNode                        */
    struct Library         *sb_NIPCBase;    /* open while the library has openers           */
    struct Library         *sb_UtilityBase;
};

#define NIPCBase                (ServicesBase->sb_NIPCBase)
#define UtilityBase             (ServicesBase->sb_UtilityBase)

#endif /* SERVICES_INTERN_H */

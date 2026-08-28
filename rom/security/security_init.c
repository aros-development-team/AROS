/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library initialization.

          The library is a ROM resident with residentpri -121, which
          genmodule turns into an RTF_AFTERDOS resident: it is initialised by
          dos.library's CliInit() once SYS: and the boot assigns exist, in
          the context of the boot process. So dos.library is available here
          and the server can be started right away (security_afterdos.c).
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/task.h>

#include <proto/security.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/symbolsets.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_segment.h"
#include "security_monitor.h"
#include "security_memory.h"
#include "security_plugins.h"

#include LC_LIBDEFS_FILE

struct SecurityBase *SecurityBaseGlobal = NULL;

static int Security_Init(LIBBASETYPEPTR secBase)
{
    D(bug(DEBUG_NAME_STR " %s(%p)\n", __func__, secBase);)

    SecurityBaseGlobal = secBase;

    secBase->sec_TaskResBase = OpenResource("task.resource");
    if (!secBase->sec_TaskResBase)
    {
        D(bug(DEBUG_NAME_STR " %s: task.resource not available\n", __func__);)
        return FALSE;
    }

    if (!InitMemory(secBase))
        return FALSE;

    InitSemaphore(&secBase->SuperSem);
    InitSemaphore(&secBase->TaskOwnerSem);
    InitSemaphore(&secBase->SegOwnerSem);
    InitSemaphore(&secBase->VolumesSem);
    InitSemaphore(&secBase->MonitorSem);
    InitSemaphore(&secBase->PluginModuleSem);

    NEWLIST((struct List *)&secBase->PluginModuleList);
    NEWLIST((struct List *)&secBase->NativeVolumes);
    secBase->FirstStartup = TRUE;
    secBase->LimitDOSSetProtection = TRUE;

    InitSegList(secBase);
    InitMonList(secBase);

    if (!InitTaskList(secBase))
    {
        CleanUpMemory(secBase);
        return FALSE;
    }

    if (!Security_AfterDOS(secBase))
    {
        D(bug(DEBUG_NAME_STR " %s: DOS side initialisation failed\n", __func__);)
        CleanUpTaskList(secBase);
        CleanUpMemory(secBase);
        return FALSE;
    }

    D(bug(DEBUG_NAME_STR " %s: initialised\n", __func__);)

    return TRUE;
}

static int Security_Expunge(LIBBASETYPEPTR secBase)
{
    D(bug(DEBUG_NAME_STR " %s(%p)\n", __func__, secBase);)

    CleanUpTaskList(secBase);
    CleanUpMemory(secBase);
    SecurityBaseGlobal = NULL;

    return TRUE;
}

/*
 * Every task that opens the library gets a fresh plugin context level,
 * see security_plugins.c.
 */
static int Security_Open(LIBBASETYPEPTR secBase)
{
    PushContext(secBase, FindTask(NULL));
    return TRUE;
}

static int Security_Close(LIBBASETYPEPTR secBase)
{
    PopContext(secBase, FindTask(NULL));
    return TRUE;
}

ADD2INITLIB(Security_Init, 0);
ADD2EXPUNGELIB(Security_Expunge, 0);
ADD2OPENLIB(Security_Open, 0);
ADD2CLOSELIB(Security_Close, 0);

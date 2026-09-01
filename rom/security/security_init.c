/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library initialization.

          Two-stage startup: the library itself is an early resident
          (residentpri 20) whose init needs only exec and task.resource -
          the base, memory, semaphores and task hooks exist before
          dos.library and the filesystem handlers, so a handler can open
          the library at mount time. The DOS-dependent part (libraries,
          catalog, the server) runs from the separate "security.boot"
          RTF_AFTERDOS resident below, at the old -121 slot, once SYS: and
          the boot assigns exist (security_afterdos.c).
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/task.h>

#include <proto/security.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/resident.h>
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

/*
 * Second stage: everything that needs dos.library. A separate RTF_AFTERDOS
 * resident (the library above initialises early), found by the kickstart
 * resident scan like acpica.post. Runs from dos.library's CliInit() at the
 * old -121 slot, before lddemon (-123).
 */
extern void securityboot_end(void);

static AROS_UFP3(APTR, SecurityBoot,
                 AROS_UFPA(struct Library *, lh, D0),
                 AROS_UFPA(BPTR, segList, A0),
                 AROS_UFPA(struct ExecBase *, SysBase, A6));

static const TEXT securityboot_namestring[] = "security.boot";
static const TEXT securityboot_versionstring[] = "security.boot 45.11\n";

const struct Resident securityboot_romtag =
{
    RTC_MATCHWORD,
    (struct Resident *)&securityboot_romtag,
    (APTR)&securityboot_end,
    RTF_AFTERDOS,
    45,
    NT_UNKNOWN,
    -121,
    (STRPTR)securityboot_namestring,
    (STRPTR)securityboot_versionstring,
    (APTR)SecurityBoot
};

static AROS_UFH3(APTR, SecurityBoot,
                 AROS_UFHA(struct Library *, lh, D0),
                 AROS_UFHA(BPTR, segList, A0),
                 AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    if (SecurityBaseGlobal)
        Security_AfterDOS(SecurityBaseGlobal);

    AROS_USERFUNC_EXIT

    return NULL;
}

void securityboot_end(void) { };
ADD2EXPUNGELIB(Security_Expunge, 0);
ADD2OPENLIB(Security_Open, 0);
ADD2CLOSELIB(Security_Close, 0);

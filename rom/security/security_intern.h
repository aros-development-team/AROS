/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: private definitions for security.library
*/
#ifndef _SECURITY_INTERN_H
#define _SECURITY_INTERN_H

#include <aros/debug.h>

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/io.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <utility/utility.h>
#include <utility/hooks.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <dos/filesystemids.h>
#include <dos/notify.h>
#include <intuition/intuition.h>
#include <libraries/locale.h>
#include <resources/task.h>

#include <aros/asmcall.h>
#include <aros/config.h>
#include <aros/symbolsets.h>

#include <libraries/security.h>

#include <string.h>

#include LC_LIBDEFS_FILE

#include "security_config.h"
#include "security_task.h"
#include "security_volumes.h"
#include "security_locale.h"

#define DEBUG_NAME_STR                  "[security.library]"

/*
 * The library base
 */
struct SecurityBase
{
    struct Library              LibNode;
    UBYTE                       Flags;
    UBYTE                       Pad;
    BPTR                        SegList;

    /* Libraries/resources we use. dos & co are only valid once
     * sec_AfterDOSDone is TRUE. */
    struct Library              *sec_TaskResBase;
    struct Library              *sec_DOSBase;
    struct Library              *sec_UtilityBase;
    struct Library              *sec_IntuitionBase;
    struct Library              *sec_LocaleBase;
    BOOL                        sec_AfterDOSDone;

    /* task.resource notification hook */
    struct Hook                 TaskNotifyHook;

    /* The Server's Process and Packet MsgPort */
    struct Process              *Server;
    struct MsgPort              *ServerPort;

    /* List of sessions */
    struct MinList              SessionsList;

    /* Tasks and their Owner(s) */
    struct SignalSemaphore      TaskOwnerSem;
    struct MinList              TaskOwnerList[TASKHASHVALUE];

    /* Segments and their Owner (setuid executables) */
    struct SignalSemaphore      SegOwnerSem;
    struct MinList              SegOwnerList;

    /* Configuration */
    struct secConfig            Config;
    BOOL                        Configured;             /* passwd file found and parsed     */
    BOOL                        LimitDOSSetProtection;  /* runtime state of the option      */

    /* Signals for Passwd File Notification and Consistency Check */
    ULONG                       NotifySig;
    ULONG                       ConsistencySig;

    /* Security violation flag */
    BOOL                        SecurityViolation;

    /* Volumes */
    struct SignalSemaphore      VolumesSem;
    struct secVolume            *Volumes;
    struct MinList              NativeVolumes;          /* fstab NATIVE entries (secNativeVolume)   */

    /* Monitoring */
    struct SignalSemaphore      MonitorSem;
    struct MinList              MonitorList;
    struct MsgPort              *MonitorPort;

    /* Task Control */
    struct MinList              Frozen;
    struct MinList              Zombies;

    /* LocaleInfo for logfile */
    struct LocaleInfo           LogInfo;

    /* You must get this one if you intend to get more than one sem. */
    struct SignalSemaphore      SuperSem;

    /* Plugins */
    struct SignalSemaphore      PluginModuleSem;
    struct MinList              PluginModuleList;

    /* Memory pool */
    struct SignalSemaphore      MemSem;
    APTR                        MemPool;

    /* Configuration directories */
    BPTR                        _pwdLock;
    BPTR                        _cfgLock;
    struct NotifyRequest        PasswdNotifyReq;
    struct NotifyRequest        GroupNotifyReq;

    /* Parsed user/group database (owned by the server) */
    struct secUserDef           *UserDefs;
    struct secGroupDef          *GroupDefs;
    char                        *Buffer;                /* general purpose line buffer      */
    char                        Key[64];
    BOOL                        FirstStartup;
};

/* Library bases are all reachable through secBase */
#define DOSBase                 ((struct DosLibrary *)secBase->sec_DOSBase)
#define UtilityBase             ((struct UtilityBase *)secBase->sec_UtilityBase)
#define IntuitionBase           ((struct IntuitionBase *)secBase->sec_IntuitionBase)
#define LocaleBase              (secBase->sec_LocaleBase)
#define TaskResBase             (secBase->sec_TaskResBase)

/* Helper to reach the library base from callbacks that only have a task */
extern struct SecurityBase *SecurityBaseGlobal;

/* Server-side entry points (security_afterdos.c) */
BOOL Security_AfterDOS(struct SecurityBase *secBase);

#endif /* _SECURITY_INTERN_H */

/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: private definitions for the security.library
    Lang: english
*/
#ifndef _SECURITY_INTERN_H
#define _SECURITY_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif
#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#define DEBUG   1

#include <exec/io.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/execbase.h>

#include <hardware/intbits.h>
#include <asm/io.h>

#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <dos/notify.h>

#include <aros/arossupportbase.h>
#include <aros/asmcall.h>
#include <aros/config.h>
#include <aros/debug.h>
#include <aros/multiboot.h>

#include <hardware/custom.h>
#include <libraries/security.h>

#include LC_LIBDEFS_FILE

#include "security_config.h"
#include "security_task.h"
#include "security_volumes.h"
#include "security_locale.h"

#define  DEBUG_NAME_STR                     "[security.library]"

struct ExecBase;
struct DosLibrary;

typedef void (*FreeFunction)(void*);

typedef APTR (*AddTaskFunc)(struct Task*, APTR,APTR,struct ExecBase*);
typedef VOID (*RemTaskFunc)(struct Task*, struct ExecBase*);
typedef APTR (*AllocMemFunc)(ULONG, ULONG, struct ExecBase *);
typedef VOID (*FreeMemFunc)(void *, ULONG, struct ExecBase *);

typedef BPTR (*LoadSegFunc)(STRPTR, struct DosLibrary *);
typedef BPTR (*NewLoadSegFunc)(STRPTR, struct TagItem *, struct DosLibrary *);
typedef BOOL (*UnLoadSegFunc)(BPTR, struct DosLibrary *);
typedef BPTR (*InternalLoadSegFunc)(BPTR, BPTR, LONG*, LONG*, struct DosLibrary *);
typedef BOOL (*InternalUnLoadSegFunc)(BPTR, FreeFunction, struct DosLibrary *);
typedef struct Process *(*CreateProcFunc)(STRPTR, LONG, BPTR, LONG, struct DosLibrary *);
typedef struct Process *(*CreateNewProcFunc)(struct TagItem *, struct DosLibrary *);
typedef LONG (*RunCommandFunc)(BPTR, ULONG, STRPTR, ULONG, struct DosLibrary *);
typedef BOOL (*SetProtectionFunc)(STRPTR, LONG, struct DosLibrary *);

/****************************************************************************

                -------------- Library Base  ------------------

 ****************************************************************************/


struct SecurityBase 
{
    struct Library              LibNode;
    UBYTE                       Flags;
    UBYTE                       Pad;
    BPTR                        SegList;

    /*
     *		The Server's Process
     */

    struct Process              *Server;

    /*
     *		The Server's Packet MsgPort
     */

    struct MsgPort              *ServerPort;

    /*
     *		List of sessions
     */

    struct MinList              SessionsList;

    /*
     *		List of Tasks and their Owner(s)
     */

    struct SignalSemaphore      TaskOwnerSem;
    struct MinList              TaskOwnerList[TASKHASHVALUE];

    /*
     *		List of memory chunks and address, size, owner.
     *		It's using the tasksemaphore since one usually wants that one
     *		too when dealing with this list.
     */

/*	struct MinList MemOwnerList[MEMHASHVALUE];*/

    /*
     *		List of Segments and their Owner
     */

    struct SignalSemaphore      SegOwnerSem;
    struct MinList              SegOwnerList;

    /*
     *		Old AddTask()/RemTask()
     */

    AddTaskFunc		        OLDAddTask;
    RemTaskFunc		        OLDRemTask;

    /*
     * 	Old AllocMem()/FreeMem()
     */
/*	
    AllocMemFunc	OLDAllocMem;
    FreeMemFunc		OLDFreeMem;
*/
    /*
     *		Old LoadSeg()/NewLoadSeg()/UnLoadSeg()/InternalLoadSeg()/
     *		InternalUnLoadSeg()/CreateProc()/CreateNewProc()/RunCommand()/
     *		SetProtection()
     */

    LoadSegFunc                 OLDLoadSeg;
    NewLoadSegFunc              OLDNewLoadSeg;
    UnLoadSegFunc               OLDUnLoadSeg;
    InternalLoadSegFunc		OLDInternalLoadSeg;
    InternalUnLoadSegFunc	OLDInternalUnLoadSeg;
    CreateProcFunc              OLDCreateProc;
    CreateNewProcFunc           OLDCreateNewProc;
    RunCommandFunc              OLDRunCommand;
    SetProtectionFunc           OLDSetProtection;

    /*
     *		Configuration
     */

    struct secConfig            Config;

    /*
     *		Signals for Passwd File Notification and Consistency Check
     */

    ULONG                       NotifySig;
    ULONG                       ConsistencySig;

    /*
     *		Security violation flag
     */

    BOOL                        SecurityViolation;

    /*
     *		MultiUser Volumes
     */

    struct SignalSemaphore      VolumesSem;
    struct secVolume            *Volumes;

    /*
     *		Monitoring
     */

    struct SignalSemaphore      MonitorSem;
    struct MinList              MonitorList;
    struct MsgPort              *MonitorPort;

    /*
     *		Task Control
     */

    struct MinList              Frozen;
    struct MinList              Zombies;

    /*
     *		LocaleInfo for logfile
     */

    struct LocaleInfo           LogInfo;

    /*
     *		You must get this one if you intend to get more than one sem.
     */

    struct SignalSemaphore      SuperSem;

    /*
     * 	Plugins
     */

    struct SignalSemaphore      PluginModuleSem;
    struct MinList              PluginModuleList;	/* List of loaded plugin modules */

    /* moved (NicJA) from globals */
    BPTR                        _pwdLock;
    BPTR                        _cfgLock;
    struct NotifyRequest        PasswdNotifyReq;
    struct NotifyRequest        GroupNotifyReq;
};

#endif /* _SECURITY_INTERN_H */

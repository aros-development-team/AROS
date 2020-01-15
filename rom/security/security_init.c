/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Security library initialization code.
    Lang: English
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/reqtools.h>

#include <proto/security.h>

#include <exec/types.h>
#include <exec/libraries.h>

#include <aros/symbolsets.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_server.h"
#include "security_segment.h"
#include "security_monitor.h"
#include "security_memory.h"

#include LC_LIBDEFS_FILE

struct ReqToolsBase *ReqToolsBase;

extern void AROS_SLIB_ENTRY(SecurityAddTask,Exec,47)(void);
extern void AROS_SLIB_ENTRY(SecurityRemTask,Exec,48)(void);

/****************************************************************************************/
static int Security_AfterDOS(LIBBASETYPEPTR secBase)
{
     D(bug( DEBUG_NAME_STR " %s(%p)\n", __func__, secBase);) 

     /*
     *  Open dos.library, intuition.library and utility.library V37+,
     *  reqtools.library V38+ and create the Server
     */

    if (!(DOSBase = (struct DosLibrary *)TaggedOpenLibrary(TAGGEDOPEN_DOS)) ||
             !(IntuitionBase = (struct IntuitionBase *)TaggedOpenLibrary(TAGGEDOPEN_INTUITION)) ||
             !(UtilityBase = (struct UtilityBase *)TaggedOpenLibrary(TAGGEDOPEN_UTILITY)) ||
             !(ReqToolsBase = (struct ReqToolsBase *)OpenLibrary("reqtools.library", 38)) ||
             !CreateServer(secBase))
    {
        D(bug( DEBUG_NAME_STR " %s: failed to open libraries\n", __func__));
        return(FALSE);
    }

    /*
     *  Open locale.library, and the default catalog for logfile output if possible
    */

    secBase->LogInfo.li_LocaleBase = OpenLibrary("locale.library",37);
    if	(secBase->LogInfo.li_LocaleBase) {
            secBase->LogInfo.li_Catalog = OpenCatalog(0,SECURITYCATALOGNAME,
                                                     OC_BuiltInLanguage,"english",
                                                     OC_Version,SECURITYCATALOGVERSION,
                                                     TAG_DONE);
    }

    Forbid();
    /*
     *  Make sure the Server is owned by the system
     */

    SetTaskExtOwner(secBase, (struct Task *)secBase->Server, (struct secExtOwner *)&RootExtOwner);

    /*
     *  Patch dos.library functions:
     *
     *			LoadSeg()
     *			NewLoadSeg()
     *			UnLoadSeg()
     *			InternalLoadSeg()
     *			InternalUnLoadSeg()
     *			CreateProc()
     *			CreateNewProc()
     *			RunCommand()
     *			SetProtection()
     *
     */
#if (0)
    secBase->OLDLoadSeg = SetFunction((struct Library *)DOSBase,
                                                                                                              (LONG)&LVOLoadSeg,
                                                                                                              (APTR)NEWLoadSeg);
    secBase->OLDNewLoadSeg = SetFunction((struct Library *)DOSBase,
                                                                                                              (LONG)&LVONewLoadSeg,
                                                                                                              (APTR)NEWNewLoadSeg);
    secBase->OLDUnLoadSeg = SetFunction((struct Library *)DOSBase,
                                                                                                              (LONG)&LVOUnLoadSeg,
                                                                                                              (APTR)NEWUnLoadSeg);
    secBase->OLDInternalLoadSeg = SetFunction((struct Library *)DOSBase,
                                                                                                              (LONG)&LVOInternalLoadSeg,
                                                                                                              (APTR)NEWInternalLoadSeg);
    secBase->OLDInternalUnLoadSeg = SetFunction((struct Library *)DOSBase,
                                                                                                                     (LONG)&LVOInternalUnLoadSeg,
                                                                                                                     (APTR)NEWInternalUnLoadSeg);
    secBase->OLDCreateProc = SetFunction((struct Library *)DOSBase,
                                                                                                    (LONG)&LVOCreateProc,
                                                                                                    (APTR)NEWCreateProc);
    secBase->OLDCreateNewProc = SetFunction((struct Library *)DOSBase,
                                                                                                            (LONG)&LVOCreateNewProc,
                                                                                                            (APTR)NEWCreateNewProc);
    secBase->OLDRunCommand = SetFunction((struct Library *)DOSBase,
                                                                                                    (LONG)&LVORunCommand,
                                                                                                    (APTR)NEWRunCommand);
    secBase->OLDSetProtection = SetFunction((struct Library *)DOSBase,
                                                                                                            (LONG)&LVOSetProtection,
                                                                                                            (APTR)NEWSetProtection);
#endif
    Permit();

    /*
     *  Activate the Server
     */

    if (!StartServer(secBase))
		return(FALSE);

	return(TRUE);
}

static int Security_Init(LIBBASETYPEPTR secBase)
{
    struct Task *task;

    D(
        bug( DEBUG_NAME_STR " %s(%p)\n", __func__, secBase);
#ifdef WANT_COMPILER_INFO
        bug( DEBUG_NAME_STR " %s starting up\n\n", __COMPILER_NAME__ " built " __DATE__ " " __TIME__);
#endif
    )

    /*
     *  Initialise Memory Management
     */

    if (!InitMemory())
        return(FALSE);

    /*
     *  Initialise the Semaphores
     */

    InitSemaphore(&secBase->SuperSem);
    InitSemaphore(&secBase->TaskOwnerSem);
    InitSemaphore(&secBase->SegOwnerSem);
    InitSemaphore(&secBase->VolumesSem);
    InitSemaphore(&secBase->MonitorSem);
    InitSemaphore(&secBase->PluginModuleSem);

    /*
     *  Initialise Task Owner, Segment Owner and Monitor Lists.
     *  Also initialise Mem Owner and Session lists.
     */

    NEWLIST((struct List *)&secBase->SessionsList);
    NEWLIST((struct List *)&secBase->PluginModuleList);

    /*
     *  Initialise Task Control
     */

    NEWLIST((struct List *)&secBase->Frozen);
    NEWLIST((struct List *)&secBase->Zombies);

    InitTaskList(secBase);
#if (0)
    InitMemList();
#endif
    InitSegList(secBase);
    InitMonList(secBase);

    /*
     *  Create Task/User list
     *
     *  All tasks have no owner.
     */
    D(bug( DEBUG_NAME_STR " %s: Initialising task/user list(s)\n", __func__);)

    Forbid();
    CreateOrphanTask(secBase, FindTask(NULL), DEFPROTECTION);
    for (task = (struct Task *)SysBase->TaskReady.lh_Head;
              task->tc_Node.ln_Succ; task = (struct Task *)task->tc_Node.ln_Succ)
              CreateOrphanTask(secBase, task, DEFPROTECTION);
    for (task = (struct Task *)SysBase->TaskWait.lh_Head;
              task->tc_Node.ln_Succ; task = (struct Task *)task->tc_Node.ln_Succ)
              CreateOrphanTask(secBase, task, DEFPROTECTION);

    /*
     *  Patch exec.library functions:
     *
     *			AddTask()
     *			RemTask()
     *
     *  This must be done inside this Forbid()/Permit() pair
     *  to prevent the birth of orphan-tasks
     */
    secBase->OLDAddTask = SetFunction((struct Library *)SysBase, -LIB_VECTSIZE * 47, (APTR) AROS_SLIB_ENTRY(SecurityAddTask, Exec, 47));
    secBase->OLDRemTask = SetFunction((struct Library *)SysBase, -LIB_VECTSIZE * 48, (APTR) AROS_SLIB_ENTRY(SecurityRemTask, Exec, 48));
    Permit();

	// TODO: only call the following if we can FindName dos.library in the library list.
	// If not we are called during rom startup - and it will be handled by the resident.

    if (!Security_AfterDOS(secBase))
		return(FALSE);

    return(TRUE);
}

/****************************************************************************************/

ADD2INITLIB(Security_Init, 0);

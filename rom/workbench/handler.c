/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    The Workbench Handler process and associated functions.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/ports.h>
#include <utility/hooks.h>
#include <dos/dosextens.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/workbench.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <workbench/workbench.h>
#include <workbench/startup.h>

#include "workbench_intern.h"
#include "support.h"
#include "handler.h"


struct HandlerContext
{
    struct MsgPort *hc_CommandPort;   /* commands from the library */
    ULONG           hc_CommandSignal;
    struct MsgPort *hc_StartupPort;   /* for WBStartup message replies */
    ULONG           hc_StartupSignal;
};

/*** Prototypes *************************************************************/
static BOOL __initialize(struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase);
static VOID __deinitialize(struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase);

static VOID __handleLaunch(struct LaunchMessage *message, struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase);
static VOID __handleDrawer(struct DrawerMessage *message, struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase);

/*** Macros *****************************************************************/
#define initialize() (__initialize(hc, WorkbenchBase))
#define deinitialize() (__deinitialize(hc, WorkbenchBase))

#define handleLaunch(message) (__handleLaunch((message), hc, WorkbenchBase))
#define handleDrawer(message) (__handleDrawer((message), hc, WorkbenchBase))

#define SET_ERROR(error) (WorkbenchBase->wb_HandlerError = (error))

/*** Entry point ************************************************************/
#undef SysBase
AROS_UFH3
(
    LONG, WorkbenchHandler,
    AROS_UFHA(STRPTR,            args,       A0),
    AROS_UFHA(ULONG,             argsLength, D0),
    AROS_UFHA(struct ExecBase *, SysBase,    A6)
)
{
    struct WorkbenchBase  *WorkbenchBase = FindTask(NULL)->tc_UserData;
    struct HandlerContext  context       = { 0 };
    struct HandlerContext *hc            = &context;
    
    /*-- Initialization ----------------------------------------------------*/
    if (initialize())
    {
        /* Prevent the library to be expunged while the handler is running */
        OpenLibrary("workbench.library", 0L);
        
        /* We're now ready to accept messages */
        WorkbenchBase->wb_HandlerPort = hc->hc_CommandPort;
        
        D(bug("Workbench Handler: entering message loop\n"));
        
        while (TRUE)
        {
            ULONG signals = Wait(hc->hc_CommandSignal | hc->hc_StartupSignal);
            D(bug("Workbench Handler: Got message(s)...\n"));
            
            if (signals & hc->hc_CommandSignal)
            {
                struct HandlerMessage *message;
                
                D(bug("Workbench Handler: Got message(s) at command port\n"));
                
                while 
                (
                    (
                        message = (struct HandlerMessage *) GetMsg
                        (
                            hc->hc_CommandPort
                        )
                    ) != NULL
                )
                {
                    switch (message->hm_Type)
                    {
                        case HM_TYPE_LAUNCH:
                            D(bug("Workbench Handler: Got launch message\n"));
                            handleLaunch((struct LaunchMessage *) message);
                            break;
                            
                        case HM_TYPE_DRAWER:
                            handleDrawer((struct DrawerMessage *) message);
                            break;
                    }
                }
            }
            
            if (signals & hc->hc_StartupSignal)
            {
                struct WBStartup *message;
                
                D(bug("Workbench Handler: Got message(s) at startup port\n"));
                
                while 
                (
                    (
                        message = (struct WBStartup *) GetMsg
                        (
                            hc->hc_StartupPort
                        )
                    ) != NULL
                )
                {
                    ULONG i;
                    
                    D(bug("Workbench Handler: Deallocating WBStartup message and arguments\n"));
                    
                    for (i = 0; i < message->sm_NumArgs; i++)
                    {
                        if (message->sm_ArgList[i].wa_Lock != NULL) UnLock(message->sm_ArgList[i].wa_Lock);
                        if (message->sm_ArgList[i].wa_Name != NULL) FreeVec(message->sm_ArgList[i].wa_Name);
                    }
                    
                    FreeMem(message, sizeof(struct WBStartup));
                }
            }
        }
    }
    
    //FIXME: shutdown not properly implemented
    
    deinitialize();
    
    return 0;
}
#define SysBase (WorkbenchBase->wb_SysBase)


static BOOL __initialize
(
    struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase
)
{
    if
    (
           (hc->hc_CommandPort = CreateMsgPort()) == NULL
        || (hc->hc_StartupPort = CreateMsgPort()) == NULL
    )
    {
        SET_ERROR(HE_MSGPORT);
        return FALSE;
    }
    
    hc->hc_CommandSignal = 1 << hc->hc_CommandPort->mp_SigBit;
    hc->hc_StartupSignal = 1 << hc->hc_StartupPort->mp_SigBit;

    return TRUE;
}

static VOID __deinitialize
(
    struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase
)
{
    if (hc->hc_CommandPort != NULL) DeleteMsgPort(hc->hc_CommandPort);
    if (hc->hc_StartupPort != NULL) DeleteMsgPort(hc->hc_StartupPort);
}

static VOID __handleLaunch
(
    struct LaunchMessage *message, 
    struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase
)
{
    struct WBStartup *startup = message->lm_StartupMessage;
    STRPTR            name    = startup->sm_ArgList[0].wa_Name;
    BPTR              lock    = startup->sm_ArgList[0].wa_Lock;
    BPTR              cd      = NULL;
    struct Process   *process = NULL;
    
    /* Free memory of launch message (don't need it anymore) */
    FreeMem(message, sizeof(struct LaunchMessage));
    
    
    /* Change directory to where the program resides */
    cd = CurrentDir(lock);
    
    /* Load the program from disk */
    startup->sm_Segment = LoadSeg(name);
    if (startup->sm_Segment != NULL)
    {
        /* Launch the program */
        process = CreateNewProcTags
        (
            NP_Seglist,     startup->sm_Segment,
            NP_Name,        name,
            NP_StackSize,   WorkbenchBase->wb_DefaultStackSize // FIXME: should be read from icon
        );
                
        if (process != NULL)
        {
            D(bug("Workbench Handler: handleLaunch: Process created successfully\n"));
                        
            /* Setup startup message */
            MESSAGE(startup)->mn_ReplyPort = hc->hc_StartupPort;
            startup->sm_Process = &process->pr_MsgPort;
            
            /* Send startup message to program */ 
            PutMsg(startup->sm_Process, startup);
        }
        else
        {
            //FIXME: free startup msg mem
            D(bug("Workbench Handler: handleLaunch: Failed to create process\n"));
        }
        
    }
    else
    {
        //FIXME: handle error
    }
    
    /* Restore current directory */
    CurrentDir(cd);
}

static VOID __handleDrawer
(
    struct DrawerMessage *message, 
    struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase
)
{
    // FIXME
}

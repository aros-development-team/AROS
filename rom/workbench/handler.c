/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    The Workbench Handler process and associated functions.
*/

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
#include "support_messages.h"
#include "handler.h"
#include "handler_support.h"

struct HandlerContext
{
    /* Ports and signals ---------------------------------------------------*/
    struct MsgPort *hc_CommandPort;        /* Command messages from the library */
    ULONG           hc_CommandSignal;      /* Signal bit field for the above */
    struct MsgPort *hc_StartupReplyPort;   /* Replies to startup messages */
    ULONG           hc_StartupReplySignal; /* Signal bit field for the above */
    struct MsgPort *hc_RelayReplyPort;     /* Replies to messages sent to the workbench application */
    ULONG           hc_RelayReplySignal;   /* Signal bit field for the above */
    struct MsgPort *hc_IntuitionPort;      /* Messages from Intuition */
    ULONG           hc_IntuitionSignal;    /* Signal bit field for the above */
    
    ULONG           hc_Signals;            /* Mask from all signals above */
};


/*** Prototypes *************************************************************/
static BOOL __Initialize_WB(struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase);
static VOID __Deinitialize_WB(struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase);

static VOID __HandleLaunch_WB(struct WBCommandMessage *message, struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase);
static VOID __HandleRelay_WB(struct WBCommandMessage *message, struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase);
static VOID __HandleIntuition_WB(struct IntuiMessage *message, struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase);

/*** Macros *****************************************************************/
#define Initialize()             (__Initialize_WB(hc, WorkbenchBase))
#define Deinitialize()           (__Deinitialize_WB(hc, WorkbenchBase))

#define HandleLaunch(message)    (__HandleLaunch_WB((message), hc, WorkbenchBase))
#define HandleRelay(message)     (__HandleRelay_WB((message), hc, WorkbenchBase))
#define HandleIntuition(message) (__HandleIntuition_WB((message), hc, WorkbenchBase))

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
    BOOL                   running       = TRUE;
    
    /*-- Initialization ----------------------------------------------------*/
    if (!Initialize()) return 20; // FIXME: report error to user somehow. displaybeep? alert?
    
    /*-- Event loop --------------------------------------------------------*/
    D(bug("Workbench Handler: entering event loop\n"));
    
    while (running)
    {
        ULONG signals = Wait(hc->hc_Signals);
        
        D(bug("Workbench Handler: Got message(s)...\n"));
        
        /*== Messages from the library =====================================*/
        if (signals & hc->hc_CommandSignal)
        {
            struct WBCommandMessage *message;
            
            D(bug("Workbench Handler: Got message(s) at command port\n"));
            
            while ((message = WBCM(GetMsg(hc->hc_CommandPort))) != NULL)
            {
                /* Handle the message */
                switch (message->wbcm_Type)
                {
                    case WBCM_TYPE_LAUNCH:
                        D(bug("Workbench Handler: Got WBCM_Launch message\n"));
                        HandleLaunch(message);
                        break;
                        
                    case WBCM_TYPE_RELAY:
                        D(bug("Workbench Handler: Got WBCM_Relay message\n"));
                        HandleRelay(message);
                        break;
                }
                
                /* Deallocate the message */
                DestroyWBCM(message);
            }
        }
        
        /*== Message replies from started WB programs ======================*/
        if (signals & hc->hc_StartupReplySignal)
        {
            struct WBStartup *message;
            
            D(bug("Workbench Handler: Got message(s) at startup reply port\n"));
            
            while ((message = WBS(GetMsg(hc->hc_StartupReplyPort))) != NULL)
            {
                if (message->sm_Message.mn_Node.ln_Type == NT_REPLYMSG)
                {
                    D(bug("Workbench Handler: Deallocating WBStartup message and arguments\n"));
                    DestroyWBS(message);
                }
                else
                {
                    /*
                        Eh, we should only get replies here. Just reply the
                        message and hope the sender will fail gracefully.
                    */
                    ReplyMsg((struct Message *) message);
                }
            }
        }
        
        /*== Message replies from the workbench application ================*/
        if (signals & hc->hc_RelayReplySignal)
        {
            struct WBHandlerMessage *message;
            
            D(bug("Workbench Handler: Got message(s) at relay reply port\n"));
            
            while ((message = WBHM(GetMsg(hc->hc_RelayReplyPort))) != NULL)
            {
                if (message->wbhm_Message.mn_Node.ln_Type == NT_REPLYMSG)
                {
                    D(bug("Workbench Handler: Deallocating WBHandlerMessage\n"));
		    if (message->wbhm_Type == WBHM_TYPE_HIDE)
		    {
		        struct IntWBHandlerMessage *iwbhm = (struct IntWBHandlerMessage *)message;
                    
		        D(bug("Workbench Handler: Replying IntuiMessage because Type = "
			      "WBHM_TYPE_HIDE"));
		        ReplyMsg((struct Message *)iwbhm->iwbhm_Data.Hide.imsg);
		    }
                    DestroyWBHM(message);
                }
                else
                {
                    /* FIXME: comment ;-) */
                    ReplyMsg((struct Message *) message);
                }
            }
            
        }
        
        /*== Messages from Intuition =======================================*/
        if (signals & hc->hc_IntuitionSignal)
        {
            struct IntuiMessage *message;
            
            D(bug("Workbench Handler: Got message(s) at intuition port\n"));
            
            while ((message = (struct IntuiMessage *) GetMsg(hc->hc_IntuitionPort)))
            {
                HandleIntuition(message); /* takes care of replying */
            }
        }
    }
    
    //FIXME: shutdown not properly implemented
    
    Deinitialize();
    
    return 0;
}
#define SysBase (WorkbenchBase->wb_SysBase)


static BOOL __Initialize_WB
(
    struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase
)
{
    /* Create message ports ------------------------------------------------*/
    if
    (
           (hc->hc_StartupReplyPort   = CreateMsgPort()) != NULL
        && (hc->hc_RelayReplyPort = CreateMsgPort()) != NULL
        && (hc->hc_IntuitionPort      = CreateMsgPort()) != NULL
    )
    {
        /* Store command port ----------------------------------------------*/
        hc->hc_CommandPort   = &(WorkbenchBase->wb_HandlerPort);
        
        /* Calculate and store signal flags --------------------------------*/
        hc->hc_CommandSignal      = 1UL << hc->hc_CommandPort->mp_SigBit;
        hc->hc_StartupReplySignal = 1UL << hc->hc_StartupReplyPort->mp_SigBit;
        hc->hc_RelayReplySignal   = 1UL << hc->hc_RelayReplyPort->mp_SigBit;
        hc->hc_IntuitionSignal    = 1UL << hc->hc_IntuitionPort->mp_SigBit;
        
        hc->hc_Signals = hc->hc_CommandSignal
                       | hc->hc_StartupReplySignal
                       | hc->hc_RelayReplySignal
                       | hc->hc_IntuitionSignal;
                       
        /* We're now ready to accept messages ------------------------------*/
        WorkbenchBase->wb_HandlerPort.mp_SigTask = FindTask(NULL);
        WorkbenchBase->wb_HandlerPort.mp_Flags   = PA_SIGNAL;  
        
        /* Make sure to process messages that arrived before we were ready -*/
        Signal(FindTask(NULL), SIGBREAKF_CTRL_F);
        
        /* Register ourselves with Intuition -------------------------------*/
        AlohaWorkbench(hc->hc_IntuitionPort);
        
        return TRUE;
    }
    else
    {
        Deinitialize(); 
        
        return FALSE;
    }
}

static VOID __Deinitialize_WB
(
    struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase
)
{
    /* Unregister ourselves with Intuition ---------------------------------*/
    AlohaWorkbench(NULL);
    
    /* We no longer accept messages ----------------------------------------*/
    WorkbenchBase->wb_HandlerPort.mp_Flags   = PA_IGNORE;
    WorkbenchBase->wb_HandlerPort.mp_SigTask = NULL;
    
    /* Deallocate message ports --------------------------------------------*/
    // FIXME: should we get & deallocate all queued messages?
    if (hc->hc_IntuitionPort != NULL)    DeleteMsgPort(hc->hc_IntuitionPort);
    if (hc->hc_RelayReplyPort != NULL)   DeleteMsgPort(hc->hc_RelayReplyPort);
    if (hc->hc_StartupReplyPort != NULL) DeleteMsgPort(hc->hc_StartupReplyPort);
}

static VOID __HandleLaunch_WB
(
    struct WBCommandMessage *message, 
    struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase
)
{
    struct WBStartup *startup = message->wbcm_Data.Launch.Startup;
    STRPTR            name    = startup->sm_ArgList[0].wa_Name;
    BPTR              lock    = startup->sm_ArgList[0].wa_Lock;
    BPTR              cd      = NULL;
    BOOL              success = FALSE;
    
    D(bug("Workbench Handler: HandleLaunch: name = %s\n", name));
    
    /* Change directory to where the program resides */
    cd = CurrentDir(lock);
    
    /* Load the program from disk */
    startup->sm_Segment = LoadSeg(name);
    if (startup->sm_Segment != NULL)
    {
        /* Launch the program */
        struct Process *process = CreateNewProcTags
        (
            NP_Seglist,     (IPTR) startup->sm_Segment,
            NP_Name,        (IPTR) name,
            NP_StackSize,          WorkbenchBase->wb_DefaultStackSize, // FIXME: should be read from icon
	    TAG_DONE
        );
                
        if (process != NULL)
        {
            D(bug("Workbench Handler: HandleLaunch: Process created successfully\n"));
            
            /* Setup startup message */
            startup->sm_Process              = &process->pr_MsgPort;
            startup->sm_Message.mn_ReplyPort = hc->hc_StartupReplyPort;
            
            /* Send startup message to program */ 
            PutMsg(startup->sm_Process, (struct Message *) startup);
            
            success = TRUE;
        }
        else
        {
            D(bug("Workbench Handler: HandleLaunch: Failed to create process\n"));
            UnLoadSeg(startup->sm_Segment); 
        }
        
    }
    else
    {
        D(bug("Workbench Handler: HandleLaunch: Failed to load segment\n"));
    }
    
    if (!success)
    {
        //FIXME: report error somehow?
        D(bug("Workbench Handler: HandleLaunch: Failed to launch program. Deallocating resources.\n"));
        DestroyWBS(startup);
    }
    
    /* Restore current directory */
    CurrentDir(cd);
}

static VOID __HandleRelay_WB
(
    struct WBCommandMessage *message,
    struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase
)
{
    struct WBHandlerMessage *relaymsg = message->wbcm_Data.Relay.Message;
    
    D(bug("Workbench Handler: HandleRelay: Relaying message to workbench application\n"));
    D(bug("Workbench Handler: HandleRelay: destination port %p\n", WorkbenchBase->wb_WorkbenchPort));
    relaymsg->wbhm_Message.mn_ReplyPort = hc->hc_RelayReplyPort;
    PutMsg(WorkbenchBase->wb_WorkbenchPort, (struct Message *) relaymsg);
}

static void __HandleIntuition_WB
(
    struct IntuiMessage *message,
    struct HandlerContext *hc, struct WorkbenchBase *WorkbenchBase
)
{
    struct IntWBHandlerMessage *iwbhm;
    
    if (message->Class == IDCMP_WBENCHMESSAGE)
    {
        switch (message->Code)
	{
	    case WBENCHOPEN:
	        iwbhm = CreateIWBHM(WBHM_TYPE_SHOW, hc->hc_RelayReplyPort);
		break;
	    case WBENCHCLOSE:
	        iwbhm = CreateIWBHM(WBHM_TYPE_HIDE, hc->hc_RelayReplyPort);
		iwbhm->iwbhm_Data.Hide.imsg = message;
		break;
            default:
	        iwbhm = NULL;
	}
        
        if (iwbhm != NULL)
        {
            PutMsg(WorkbenchBase->wb_WorkbenchPort, (struct Message *)iwbhm);
	
	    /* Don't reply immediately if we're asked to close the WB,
	       we'll do it when handling replied message, so that the WB
	       will have a chance to close its windows.  */ 
	    if (message->Code == WBENCHCLOSE)
                return;
        }
    }
    
    ReplyMsg((struct Message *) message);
}

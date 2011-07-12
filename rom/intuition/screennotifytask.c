/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include "intuition_intern.h"

#include "screennotifytask.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/**************************************************************************************************/

/***************************
**  DefaultScreennotifyHandler()  **
***************************/
void DefaultScreennotifyHandler(struct ScreennotifyTaskParams *taskparams)
{
    struct MsgPort          *port = NULL;
    BOOL            	     success = FALSE;

    if ((port = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        port->mp_Node.ln_Type   = NT_MSGPORT;
        port->mp_Flags      	= PA_SIGNAL;
        port->mp_SigBit     	= AllocSignal(-1);
        port->mp_SigTask    	= FindTask(0);
        NEWLIST(&port->mp_MsgList);

        success = TRUE;

    } /* if ((mem = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR))) */

    if (success)
    {
        taskparams->ScreennotifyHandlerPort = port;
        taskparams->success = TRUE;
    }

    Signal(taskparams->Caller, SIGF_INTUITION);

    if (!success)
    {
        D(bug("DefaultScreennotifyHandler: initialization failed. waiting for parent task to kill me.\n"));
        Wait(0);
    }

    D(bug("DefaultScreennotifyHandler: initialization ok. Now waiting for messages from Intuition.\n"));

    for(;;)
    {
        struct ScreenNotifyMessage *msg;

        WaitPort(port);
        while((msg = (struct ScreenNotifyMessage *) GetMsg(port)))
        {
            FreeMem((APTR) msg, sizeof(struct ScreenNotifyMessage));
        } /* while((msg = (struct ScreenNotifyMessage *)GetMsg(port))) */

    } /* for(;;) */
}

/**************************************************************************************************/

/*******************************
**  InitDefaultScreennotifyHandler()  **
*******************************/
BOOL InitDefaultScreennotifyHandler(struct IntuitionBase *IntuitionBase)
{
    struct ScreennotifyTaskParams   params;
    struct Task                    *task;
    BOOL            	            result = FALSE;

    params.intuitionBase = IntuitionBase;
    params.Caller    = FindTask(NULL);
    params.success   = FALSE;

    SetSignal(0, SIGF_INTUITION);

    task = NewCreateTask(TASKTAG_NAME	  , SCREENNOTIFYTASK_NAME,
    			 TASKTAG_PRI	  , SCREENNOTIFYTASK_PRIORITY,
    			 TASKTAG_STACKSIZE, SCREENNOTIFYTASK_STACKSIZE,
    			 TASKTAG_PC	  , DefaultScreennotifyHandler,
    			 TASKTAG_ARG1	  , &params,
    			 TAG_DONE);

    if (task)
    {
        Wait(SIGF_INTUITION);

        if (params.success)
        {
            result = TRUE;
            GetPrivIBase(IntuitionBase)->ScreenNotifyReplyPort = params.ScreennotifyHandlerPort;
        }
        else
        {
            RemTask(task);
        }

    } /* if ((task = CreateScreennotifyHandlerTask(&params, IntuitionBase))) */

    return result;
}


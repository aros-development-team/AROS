/*
    Copyright  1995-2007, The AROS Development Team. All rights reserved.
    $Id: screennotifytask.c 20886 2006-12-31 18:51:21Z dariusb $
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

void DefaultScreennotifyHandler(struct ScreennotifyTaskParams *taskparams);

/**************************************************************************************************/
/******************************
**  CreateScreennotifyHandlerTask()  **
******************************/
struct Task *CreateScreennotifyHandlerTask(APTR taskparams, struct IntuitionBase *IntuitionBase)
{
    struct Task *task;
    APTR    stack;

    task = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (task)
    {
        NEWLIST(&task->tc_MemEntry);
        task->tc_Node.ln_Type = NT_TASK;
        task->tc_Node.ln_Name = SCREENNOTIFYTASK_NAME;
        task->tc_Node.ln_Pri = SCREENNOTIFYTASK_PRIORITY;

        stack = AllocMem(SCREENNOTIFYTASK_STACKSIZE, MEMF_PUBLIC);
        if(stack != NULL)
        {
            task->tc_SPLower=stack;
            task->tc_SPUpper=(UBYTE *)stack + SCREENNOTIFYTASK_STACKSIZE;

    	    {
	    	struct TagItem tags[] =
		{
		    {TASKTAG_ARG1, (IPTR)taskparams },
		    {TAG_DONE	    	    	    }
		};
		
    	    #if AROS_STACK_GROWS_DOWNWARDS
    		task->tc_SPReg = (UBYTE *)task->tc_SPUpper-SP_OFFSET;
    	    #else
        	task->tc_SPReg=(UBYTE *)task->tc_SPLower+SP_OFFSET;
    	    #endif

        	if(NewAddTask(task, DefaultScreennotifyHandler, NULL, tags) != NULL)
        	{
                    /* Everything went OK */
                    return (task);
        	}
	    
	    }
            FreeMem(stack, SCREENNOTIFYTASK_STACKSIZE);

        } /* if(stack != NULL) */
        FreeMem(task,sizeof(struct Task));

    } /* if (task) */
    return (NULL);

}



/**************************************************************************************************/

/***************************
**  DefaultScreennotifyHandler()  **
***************************/
void DefaultScreennotifyHandler(struct ScreennotifyTaskParams *taskparams)
{
    struct IntuitionBase    *IntuitionBase = taskparams->intuitionBase;
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

    if ((task = CreateScreennotifyHandlerTask(&params, IntuitionBase)))
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


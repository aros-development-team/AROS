#define AROS_ALMOST_COMPATIBLE 1
#include <proto/exec.h>
#include <proto/timer.h>
#include <exec/lists.h>
#include <exec/interrupts.h>
#include <exec/alerts.h>
#include <devices/inputevent.h>
#include <devices/input.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <aros/asmcall.h>

#include "input_intern.h"

#define DEBUG 0
#include <aros/debug.h>

/**********************
**  ForwardEvents()  **
**********************/
/* Forwards a chain of events to the inputhandlers */
VOID ForwardQueuedEvents(struct inputbase *InputDevice)
{
   struct InputEvent *ie_chain;
   struct Interrupt *ihiterator;
   
   ie_chain = GetEventsFromQueue(InputDevice);
   if (ie_chain)
   {
   	ForeachNode(&(InputDevice->HandlerList), ihiterator)
    	{
/*	    D(bug("ipe: calling inputhandler %s at %p\n",
	    		ihiterator->is_Node.ln_Name, ihiterator->is_Code));
*/		
            ie_chain = AROS_UFC2(struct InputEvent *, ihiterator->is_Code,
		    AROS_UFCA(struct InputEvent *,  ie_chain,          	    A0),
		    AROS_UFCA(APTR,                 ihiterator->is_Data,    A1));

//	    D(bug("ipe: returned from inputhandler\n"));
	} /* for each input handler */
	
    } 
    
    return;
}

/***********************************
** Input device task entry point  **
***********************************/
void ProcessEvents (struct IDTaskParams *taskparams)
{
    struct inputbase *InputDevice = taskparams->InputDevice;
    ULONG commandsig, wakeupsigs;
    struct MsgPort *timermp;
    struct timerequest *timerio;
    struct Library *TimerBase;
    
    /* Initializing command msgport */
    InputDevice->CommandPort->mp_Flags	 = PA_SIGNAL;
    InputDevice->CommandPort->mp_SigTask = FindTask(NULL);
    
    /* This will always succeed, as this task just has been created */
    InputDevice->CommandPort->mp_SigBit = AllocSignal(-1L);
    NEWLIST( &(InputDevice->CommandPort->mp_MsgList) );

    /* Tell the task that created us, that we are finished initializing */
    Signal(taskparams->Caller, taskparams->Signal);
    
    /* Opening the timer device */
    timermp = CreateMsgPort();
    if (!timermp)
    	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
    	
    timerio = (struct timerequest *)CreateIORequest(timermp, sizeof(struct timerequest));
    if (!timerio)
        Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);

    if ( 0 != OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest *)timerio, 0))
    	Alert(AT_DeadEnd | AG_OpenDev | AN_Unknown);
    
    TimerBase = (struct Library *)timerio->tr_node.io_Device;

    
    commandsig = 1 << InputDevice->CommandPort->mp_SigBit;

    for (;;)
    {

//	D(bug("id : waiting for wakeup-call\n"));
	wakeupsigs = Wait (commandsig);
	
	if (wakeupsigs & commandsig)
	{
	    struct IOStdReq *ioreq;
	    /* Get all commands from the port */
	    while ( (ioreq = (struct IOStdReq *)GetMsg(InputDevice->CommandPort)) )
	    {
//	    D(bug("id task: processing sommand %d\n", ioreq->io_Command));
	    	
	    switch (ioreq->io_Command)
	    {
	    
	    	
	    case IND_ADDHANDLER:
    	    	Enqueue((struct List *)&(InputDevice->HandlerList),
    	    		(struct Node *)ioreq->io_Data);
    	        break;
    	    	    
    	    case IND_REMHANDLER:
    	    	Remove((struct Node *)ioreq->io_Data);
    	    	break;
    	    	    
    	    case IND_WRITEEVENT: {
    	        struct InputEvent *ie;
    	        
    	        ie = (struct InputEvent *)ioreq->io_Data; 
    	    	/* Add a timestamp to the event */
//    	    	D(bug("id: Getting system time\n"));
    	    	GetSysTime( &(ie->ie_TimeStamp ));

    	    	D(bug("id: %d\n", ie->ie_Class));
    	    	
    	    	/* Add event to queue */
    	    	
    	    	AddEQTail((struct InputEvent *)ioreq->io_Data, InputDevice);
    	    	
    //	    	D(bug("id: Forwarding events\n"));
    	    	
    	    	/* Forward event (and possible others in the queue) */
    	    	ForwardQueuedEvents(InputDevice);
//    	    	D(bug("id: Events forwarded\n"));

    	    	} break;
    	    	
	    	    
	    } /* switch (IO command) */

    		ReplyMsg((struct Message *)ioreq);
    		
	    } /* while (messages in the command port) */
	    
	} /* if (IO command received) */
	
    } /* Forever */
   
} /* ProcessEvents */


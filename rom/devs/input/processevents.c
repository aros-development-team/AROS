#define AROS_ALMOST_COMPATIBLE 1
#include <proto/exec.h>
#include <proto/timer.h>
#include <exec/lists.h>
#include <exec/interrupts.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <devices/inputevent.h>
#include <devices/input.h>
#include <devices/timer.h>
#include <devices/keyboard.h>
#include <intuition/intuition.h>
#include <aros/asmcall.h>

#include "input_intern.h"

#define SEND_KBD_REQUEST(kbdio, kbdie)			\
    kbdio->io_Command = KBD_READEVENT;			\
    kbdio->io_Data = (APTR)kbdie;			\
    kbdio->io_Length = sizeof (struct InputEvent);	\
    SendIO((struct IORequest *)kbdio)

#define DEBUG 1
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
    ULONG commandsig, kbdsig, wakeupsigs;
    struct MsgPort *timermp;
    struct timerequest *timerio;
    struct MsgPort *kbdmp;
    struct IOStdReq *kbdio;
    struct InputEvent *kbdie;
    
    struct Library *TimerBase;
    
    /* Initializing command msgport */
    InputDevice->CommandPort->mp_Flags	 = PA_SIGNAL;
    InputDevice->CommandPort->mp_SigTask = FindTask(NULL);
    
    /* This will always succeed, as this task just has been created */
    InputDevice->CommandPort->mp_SigBit = AllocSignal(-1L);
    NEWLIST( &(InputDevice->CommandPort->mp_MsgList) );

    
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

    /* Open the keyboard.device */
    kbdmp = CreateMsgPort();
    if (!kbdmp)
    	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
    	
    kbdio = (struct IOStdReq *)CreateIORequest(kbdmp, sizeof(struct IOStdReq));
    if (!kbdio)
        Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);

    if ( 0 != OpenDevice("keyboard.device", 0, (struct IORequest *)kbdio, 0))
    	Alert(AT_DeadEnd | AG_OpenDev | AN_Unknown);
    
    
    kbdie = AllocMem(sizeof (struct InputEvent), MEMF_PUBLIC | MEMF_CLEAR);
    if (!kbdie)
        Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
	
    /* Send an initial request to the keyboard device */
    SEND_KBD_REQUEST(kbdio, kbdie);
    
    commandsig = 1 << InputDevice->CommandPort->mp_SigBit;
    
    kbdsig = 1 << kbdmp->mp_SigBit;
    

    /* Tell the task that created us, that we are finished initializing */
    Signal(taskparams->Caller, taskparams->Signal);
    for (;;)
    {

//	D(bug("id : waiting for wakeup-call\n"));
	wakeupsigs = Wait (commandsig|kbdsig);
	D(bug("Wakeup sig: %x, cmdsig: %x, kbdsig: %x\n"
		, wakeupsigs, commandsig, kbdsig));
	
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
	else if (wakeupsigs & kbdsig)
	{
	    
	    GetMsg(kbdmp); /* Only one message */
	    if (kbdio->io_Error != 0)
	    	continue;
	    
	    AddEQTail((struct InputEvent *)kbdio->io_Data, InputDevice);
	    /* New event from keyboard device */
    	    D(bug("id: Keyboard event\n"));
	    /* Add event to queue */
    	    	
    	    D(bug("id: Forwarding events\n"));
    	    	
    	    	/* Forward event (and possible others in the queue) */
	    ForwardQueuedEvents(InputDevice);
    	    D(bug("id: Events forwarded\n"));

	    /* Wit for some more events */
	    SEND_KBD_REQUEST(kbdio, kbdie);
	}
	
    } /* Forever */
   
} /* ProcessEvents */


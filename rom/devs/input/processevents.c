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
#include <devices/gameport.h>
#include <intuition/intuition.h>
#include <aros/asmcall.h>

#include "input_intern.h"

#define SEND_INPUT_REQUEST(io, ie, cmd)	\
    io->io_Command = cmd;			\
    io->io_Data = (APTR)ie;			\
    io->io_Length = sizeof (struct InputEvent);	\
    SendIO((struct IORequest *)io)
    
    
#define SEND_KBD_REQUEST(kbdio, kbdie)	SEND_INPUT_REQUEST(kbdio, kbdie, KBD_READEVENT)
#define SEND_GPD_REQUEST(gpdio, gpdie) SEND_INPUT_REQUEST(gpdio, gpdie, GPD_READEVENT)

#define SEND_TIMER_REQUEST(timerio)			\
	timerio->tr_node.io_Command = TR_ADDREQUEST;	\
	timerio->tr_time.tv_secs = 0;			\
	timerio->tr_time.tv_micro = 100000;		\
	SendIO((struct IORequest *)timerio)

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
    ULONG commandsig, kbdsig, wakeupsigs, gpdsig, timersig;
    struct MsgPort *timermp;
    struct timerequest *timerio;


    struct MsgPort *kbdmp, *gpdmp;
    struct IOStdReq *kbdio, *gpdio;
    struct InputEvent *kbdie, *gpdie;
    
    
    struct Library *TimerBase;
    
    /* Initializing command msgport */
    InputDevice->CommandPort->mp_Flags	 = PA_SIGNAL;
    InputDevice->CommandPort->mp_SigTask = FindTask(NULL);
    
    /* This will always succeed, as this task just has been created */
    InputDevice->CommandPort->mp_SigBit = AllocSignal(-1L);
    NEWLIST( &(InputDevice->CommandPort->mp_MsgList) );

    
    /************** Open timer.device *******************/
    
    timermp = CreateMsgPort();
    if (!timermp)
    	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
    	
    timerio = (struct timerequest *)CreateIORequest(timermp, sizeof(struct timerequest));
    if (!timerio)
        Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);

    if ( 0 != OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest *)timerio, 0))
    	Alert(AT_DeadEnd | AG_OpenDev | AN_Unknown);
    
    TimerBase = (struct Library *)timerio->tr_node.io_Device;

    /************** Open keyboard.device *******************/
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
	

    /************** Open gameport.device *******************/
    gpdmp = CreateMsgPort();
    if (!gpdmp)
    	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
    	
    gpdio = (struct IOStdReq *)CreateIORequest(gpdmp, sizeof(struct IOStdReq));
    if (!gpdio)
        Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);

    if ( 0 != OpenDevice("gameport.device", 0, (struct IORequest *)gpdio, 0))
    	Alert(AT_DeadEnd | AG_OpenDev | AN_Unknown);
    
    
    gpdie = AllocMem(sizeof (struct InputEvent), MEMF_PUBLIC | MEMF_CLEAR);
    if (!gpdie)
        Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);


	
    /* Send an initial request to the keyboard device */

    SEND_KBD_REQUEST(kbdio, kbdie);
    
    /* .. and to the gameport.device */
    SEND_GPD_REQUEST(gpdio, gpdie);
    
    /* .. and to the timer device */
    SEND_TIMER_REQUEST(timerio);
    
    commandsig = 1 << InputDevice->CommandPort->mp_SigBit;
    
    kbdsig = 1 << kbdmp->mp_SigBit;
    gpdsig = 1 << gpdmp->mp_SigBit;
    timersig = 1 << timermp->mp_SigBit;

    /* Tell the task that created us, that we are finished initializing */
    Signal(taskparams->Caller, taskparams->Signal);
    for (;;)
    {

	wakeupsigs = Wait (commandsig | kbdsig | gpdsig | timersig);
	D(bug("Wakeup sig: %x, cmdsig: %x, kbdsig: %x\n, timersig: %x"
		, wakeupsigs, commandsig, kbdsig, timersig));
	

	
	if (wakeupsigs & timersig)
	{
	    struct InputEvent timer_ie;
	    
	    GetMsg(timermp);

	    timer_ie.ie_Class = IECLASS_TIMER;
	    timer_ie.ie_NextEvent = NULL;
	    
	    /* Add a timestamp to the event */
	    GetSysTime( &(timer_ie.ie_TimeStamp ));
	    
	    AddEQTail(&timer_ie, InputDevice);
	    ForwardQueuedEvents(InputDevice);
	    
	    
	    SEND_TIMER_REQUEST(timerio);
	}
	
	if (wakeupsigs & commandsig)
	{
	    struct IOStdReq *ioreq;
	    /* Get all commands from the port */
	    while ( (ioreq = (struct IOStdReq *)GetMsg(InputDevice->CommandPort)) )
	    {
	    	
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
		    GetSysTime( &(ie->ie_TimeStamp ));

		    D(bug("id: %d\n", ie->ie_Class));
    	    	
		    /* Add event to queue */
    	    	
		    AddEQTail((struct InputEvent *)ioreq->io_Data, InputDevice);
    	    	
    	    	
    	    	/* Forward event (and possible others in the queue) */
		    ForwardQueuedEvents(InputDevice);


		    } break;
    	    	
	    	    
		} /* switch (IO command) */

    		ReplyMsg((struct Message *)ioreq);
    		
	    } /* while (messages in the command port) */
	   
	    
	} /* if (IO command received) */
	if (wakeupsigs & kbdsig)
	{
	    
	    GetMsg(kbdmp); /* Only one message */
	    if (kbdio->io_Error != 0)
	    	continue;
	    
	    #define KEY_QUALIFIERS (IEQUALIFIER_LSHIFT     | IEQUALIFIER_RSHIFT   | \
				    IEQUALIFIER_CAPSLOCK   | IEQUALIFIER_CONTROL  | \
				    IEQUALIFIER_RALT       | IEQUALIFIER_LALT     | \
				    IEQUALIFIER_RCOMMAND   | IEQUALIFIER_RCOMMAND | \
				    IEQUALIFIER_NUMERICPAD | IEQUALIFIER_REPEAT)

	    InputDevice->ActQualifier &= ~KEY_QUALIFIERS;
	    InputDevice->ActQualifier |= (((struct InputEvent *)kbdio->io_Data)->ie_Qualifier & KEY_QUALIFIERS);
	     
	    /* Add event to queue */
	    AddEQTail((struct InputEvent *)kbdio->io_Data, InputDevice);
	    /* New event from keyboard device */
    	    D(bug("id: Keyboard event\n"));
    	    	
    	    	
    	    D(bug("id: Events forwarded\n"));
    	    	/* Forward event (and possible others in the queue) */
	    ForwardQueuedEvents(InputDevice);
    	    D(bug("id: Events forwarded\n"));

	    /* Wit for some more events */
	    SEND_KBD_REQUEST(kbdio, kbdie);
	}
	if (wakeupsigs & gpdsig)
	{
	    GetMsg(gpdmp); /* Only one message */
	    if (gpdio->io_Error != 0)
	    	continue;
	    
	    /* FIXME: Maybe this should be done in gameport.device */

	#if 0
	    /* this should work once qualifiers are handled by gameport.device */
		 
	    #define MOUSE_QUALIFIERS (IEQUALIFIER_LEFTBUTTON | IEQUALIFIER_RBUTTON | \
	    			      IEQUALIFIER_MIDBUTTON)
	
	    InputDevice->ActQualifier &= ~MOUSE_QUALIFIERS;
	    InputDevice->ActQualifier |= (((struct InputEvent *)gpdio->io_Data)->ie_Qualifier & MOUSE_QUALIFIERS);
	#else
	    
	    switch( ((struct InputEvent *)gpdio->io_Data)->ie_Code )
	    {
	        case SELECTDOWN:
		    InputDevice->ActQualifier |= IEQUALIFIER_LEFTBUTTON;
		    break;
		    
		case SELECTUP:
		    InputDevice->ActQualifier &= ~IEQUALIFIER_LEFTBUTTON;
		    break;

	        case MIDDLEDOWN:
		    InputDevice->ActQualifier |= IEQUALIFIER_MIDBUTTON;
		    break;
		    
		case MIDDLEUP:
		    InputDevice->ActQualifier &= ~IEQUALIFIER_MIDBUTTON;
		    break;

	        case MENUDOWN:
		    InputDevice->ActQualifier |= IEQUALIFIER_RBUTTON;
		    break;
		    
		case MENUUP:
		    InputDevice->ActQualifier &= ~IEQUALIFIER_RBUTTON;
		    break;
		    
	    } /* switch( ((struct InputEvent *)gpdio->io_Data)->ie_Code) */
	    
	#endif

	    /* Gameport just returns the frame count since the last
	       report in ie_TimeStamp.tv_secs; we therefore must add
	       a real timestamp ourselves */
	    GetSysTime(&gpdie->ie_TimeStamp);	    
	    
	    /* Add event to queue */
	    AddEQTail((struct InputEvent *)gpdio->io_Data, InputDevice);
	    /* New event from gameport device */
    	    D(bug("id: Gameport event\n"));

    	    	
    	    D(bug("id: Forwarding events\n"));
    	    	/* Forward event (and possible others in the queue) */
	    ForwardQueuedEvents(InputDevice);
    	    D(bug("id: Events forwarded\n"));

	    /* Wit for some more events */
	    SEND_GPD_REQUEST(gpdio, gpdie);
	
	}
	
    } /* Forever */
   
} /* ProcessEvents */


/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/graphics.h>

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

#define SEND_KEYTIMER_REQUEST(timerio,time)		\
	timerio->tr_node.io_Command = TR_ADDREQUEST;	\
	timerio->tr_time = time;			\
	SendIO((struct IORequest *)timerio)

#define ABORT_KEYTIMER_REQUEST \
	if (!CheckIO(&keytimerio->tr_node)) AbortIO(&keytimerio->tr_node); \
	WaitIO(&keytimerio->tr_node); \
	SetSignal(0, keytimersig); 

#define DEBUG 0
#include <aros/debug.h>

AROS_UFH3S(void, ResetHandler,
    AROS_UFHA(struct inputbase *, InputDevice, A1),
    AROS_UFHA(APTR, code, A5),
    AROS_UFHA(struct ExecBase *, sysBase, A6))
{
    AROS_USERFUNC_INIT
    
    if (InputDevice->ResetSig)
    {
    	Signal(InputDevice->InputTask, InputDevice->ResetSig);
    }

    AROS_USERFUNC_EXIT
}
 
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
	    D(bug("ipe: calling inputhandler %s at %p\n",
	    		ihiterator->is_Node.ln_Name, ihiterator->is_Code));
		
            ie_chain = AROS_UFC2(struct InputEvent *, ihiterator->is_Code,
		    AROS_UFCA(struct InputEvent *,  ie_chain,          	    A0),
		    AROS_UFCA(APTR,                 ihiterator->is_Data,    A1));

	    D(bug("ipe: returned from inputhandler\n"));

	} /* for each input handler */
	
    } 
    
    return;
}


/***********************************
** Input device task entry point  **
***********************************/
void ProcessEvents (struct inputbase *InputDevice)
{
    ULONG   	    	     commandsig, kbdsig, wakeupsigs;
    ULONG   	    	     gpdsig, timersig, keytimersig;
    struct MsgPort  	    *timermp, *keytimermp;
    struct timerequest      *timerio, *keytimerio;

    struct MsgPort  	    *kbdmp, *gpdmp;
    struct IOStdReq 	    *kbdio, *gpdio;
    struct InputEvent 	    *kbdie, *gpdie, keyrepeatie;
    struct Interrupt	    resethandler;
    struct Library  	    *TimerBase;
    
    struct GamePortTrigger  mouseTrigger =
    {
	GPTF_DOWNKEYS | GPTF_UPKEYS,
	9999,			/* We dont really care about time triggers */
	0,			/* Report any mouse change */
	0
    };

    BYTE    	    	    controllerType = GPCT_MOUSE;
    BYTE    	    	    keyrepeat_state = 0;
    
    /************** Open timer.device *******************/
    
    timermp    = CreateMsgPort();
    keytimermp = CreateMsgPort();
    
    if (!timermp || !keytimermp)
    	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
    	
    timerio    = (struct timerequest *)CreateIORequest(timermp   , sizeof(struct timerequest));
    keytimerio = (struct timerequest *)CreateIORequest(keytimermp, sizeof(struct timerequest));
    if (!timerio || !keytimerio)
        Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);

    if ( 0 != OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest *)timerio, 0))
    	Alert(AT_DeadEnd | AG_OpenDev | AN_Unknown);
    
    TimerBase = (struct Library *)timerio->tr_node.io_Device;

    *keytimerio = *timerio;
    keytimerio->tr_node.io_Message.mn_ReplyPort = keytimermp;
    
    /************** Open keyboard.device *******************/
    kbdmp = CreateMsgPort();
    if (!kbdmp)
    	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
    	
    kbdio = (struct IOStdReq *)CreateIORequest(kbdmp, sizeof(struct IOStdReq));
    if (!kbdio)
        Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);

    if ( 0 != OpenDevice("keyboard.device", 0, (struct IORequest *)kbdio, 0))
    	Alert(AT_DeadEnd | AG_OpenDev | AN_Unknown);
    
    /* Install RESET Handler */
    
    InputDevice->ResetSig = 1L << AllocSignal(-1);
    
    resethandler.is_Node.ln_Name = "input.device reset handler";
    resethandler.is_Node.ln_Type = NT_INTERRUPT;
    resethandler.is_Node.ln_Pri = -128;
    
    resethandler.is_Code = (VOID (*)())ResetHandler;
    resethandler.is_Data = InputDevice;
    
    kbdio->io_Command = KBD_ADDRESETHANDLER;
    kbdio->io_Data = &resethandler;
    DoIO((struct IORequest *)kbdio);
    
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

    /* Set the controller type */
    gpdio->io_Command = GPD_SETCTYPE;
    gpdio->io_Data = (APTR)&controllerType;
    gpdio->io_Length = sizeof(BYTE);
    DoIO((struct IORequest *)gpdio);

    /* Set the gameport trigger */
    gpdio->io_Command = GPD_SETTRIGGER;
    gpdio->io_Data = &mouseTrigger;
    gpdio->io_Length = sizeof(struct GamePortTrigger);
    DoIO((struct IORequest *)gpdio);
    
    gpdie = AllocMem(sizeof (struct InputEvent), MEMF_PUBLIC | MEMF_CLEAR);
    if (!gpdie)
        Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
	

    /* Send an initial request to the keyboard device */
    SEND_KBD_REQUEST(kbdio, kbdie);
    
    /* .. and to the gameport.device */
    SEND_GPD_REQUEST(gpdio, gpdie);
    
    /* .. and to the timer device */
    SEND_TIMER_REQUEST(timerio);
    
    commandsig = 1 << InputDevice->CommandPort.mp_SigBit;
    
    kbdsig      = 1 << kbdmp->mp_SigBit;
    gpdsig      = 1 << gpdmp->mp_SigBit;
    timersig    = 1 << timermp->mp_SigBit;
    keytimersig = 1 << keytimermp->mp_SigBit;
    
    for (;;)
    {
	wakeupsigs = Wait (commandsig 	    	 |
	    	    	   kbdsig     	    	 |
			   gpdsig   	    	 |
			   timersig 	    	 |
			   keytimersig      	 |
			   InputDevice->ResetSig |
			   SIGBREAKF_CTRL_F);

	D(bug("Wakeup sig: %x, cmdsig: %x, kbdsig: %x\n, timersig: %x"
		, wakeupsigs, commandsig, kbdsig, timersig));
	
	if (wakeupsigs & SIGBREAKF_CTRL_F)
	{
	    struct InputEvent null_ie;
	    
	    null_ie.ie_NextEvent 	 = NULL;
	    null_ie.ie_Class 		 = IECLASS_NULL;
	    null_ie.ie_SubClass 	 = 0;
	    null_ie.ie_Code 		 = 0;
	    null_ie.ie_Qualifier 	 = 0;
	    null_ie.ie_position.ie_addr  = 0;

	    /* Add a timestamp to the event */
	    GetSysTime( &(null_ie.ie_TimeStamp ));
	    AddEQTail(&null_ie, InputDevice);
	    ForwardQueuedEvents(InputDevice);	    
	}
	
	if (wakeupsigs & timersig)
	{
	    struct InputEvent timer_ie;
	    
	    GetMsg(timermp);

	    timer_ie.ie_NextEvent 	 = NULL;
	    timer_ie.ie_Class 		 = IECLASS_TIMER;
	    timer_ie.ie_SubClass 	 = 0;
	    timer_ie.ie_Code 		 = 0;
	    timer_ie.ie_Qualifier 	 = InputDevice->ActQualifier;
	    timer_ie.ie_position.ie_addr = 0;
	    
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
	    while ((ioreq = (struct IOStdReq *)GetMsg(&InputDevice->CommandPort)))
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
    	    	    
		case IND_SETMTRIG:
		    break;

		case IND_SETMTYPE:
		    break;

		case IND_WRITEEVENT: {
		    struct InputEvent *ie;
		    
		    ie = (struct InputEvent *)ioreq->io_Data;
		    
		    ie->ie_NextEvent = NULL; 
		    /* Add a timestamp to the event */
		    GetSysTime( &(ie->ie_TimeStamp ));
		    
		    D(bug("id: %d\n", ie->ie_Class));
		    
		    /* Add event to queue */
		    AddEQTail((struct InputEvent *)ioreq->io_Data,
			      InputDevice);
		    
		    /* Forward event (and possible others in the queue) */
		    ForwardQueuedEvents(InputDevice);
		} break;
    	    	
		case IND_SETTHRESH:
		    InputDevice->KeyRepeatThreshold = ((struct timerequest *)ioreq)->tr_time;
		    break;
		    
		case IND_SETPERIOD:
		    InputDevice->KeyRepeatInterval  = ((struct timerequest *)ioreq)->tr_time;
		    break;
		    
		} /* switch (IO command) */
		
    		ReplyMsg((struct Message *)ioreq);
    		
	    } /* while (messages in the command port) */
	    
	} /* if (IO command received) */

	if (wakeupsigs & keytimersig)
	{
	    struct InputEvent ie;
	    
	    GetMsg(keytimermp);
	    
	    keyrepeat_state = 2;

	    ie = keyrepeatie; /* InputHandlers can change inputevents, so send a clone!! */	
	    ie.ie_NextEvent = NULL; /* !! */
	    ie.ie_Qualifier |= IEQUALIFIER_REPEAT;
	    GetSysTime(&ie.ie_TimeStamp);	    

	    AddEQTail(&ie, InputDevice);

	    /* Forward event (and possible others in the queue) */
	    ForwardQueuedEvents(InputDevice);

	    SEND_KEYTIMER_REQUEST(keytimerio, InputDevice->KeyRepeatInterval);
	        
	} /* if (wakeupsigs & keytimersig) */
	
	if (wakeupsigs & kbdsig)
	{
	    GetMsg(kbdmp); /* Only one message */
	    if (kbdio->io_Error != 0)
	    	continue;
	    
#define KEY_QUALIFIERS (IEQUALIFIER_LSHIFT     | IEQUALIFIER_RSHIFT   | \
			IEQUALIFIER_CAPSLOCK   | IEQUALIFIER_CONTROL  | \
			IEQUALIFIER_RALT       | IEQUALIFIER_LALT     | \
			IEQUALIFIER_RCOMMAND   | IEQUALIFIER_RCOMMAND | \
			IEQUALIFIER_NUMERICPAD /* | IEQUALIFIER_REPEAT */)
		    
	    InputDevice->ActQualifier &= ~KEY_QUALIFIERS;
	    InputDevice->ActQualifier |= (kbdie->ie_Qualifier & KEY_QUALIFIERS);
	    
	    /* Add event to queue */
	    AddEQTail(kbdie, InputDevice);

	    if (!IsQualifierKey(kbdie->ie_Code))
	    {
	        if (keyrepeat_state > 0)
		{
		    ABORT_KEYTIMER_REQUEST;
		    keyrepeat_state = 0;
		}

		if (!(kbdie->ie_Code & IECODE_UP_PREFIX))
		{
		    if (IsRepeatableKey(kbdie->ie_Code))
		    {
		        keyrepeatie = *kbdie;

			SEND_KEYTIMER_REQUEST(keytimerio, InputDevice->KeyRepeatThreshold);
			keyrepeat_state = 1;

		    }
		}
	    } /* if (!IsQualifierKey(kbdie->ie_Code)) */
	    
	    /* New event from keyboard device */
    	    D(bug("id: Keyboard event\n"));
	    D(bug("id: Events forwarded\n"));

	    /* Forward event (and possible others in the queue) */
	    ForwardQueuedEvents(InputDevice);
    	    D(bug("id: Events forwarded\n"));
	    
	    /* Wit for some more events */
	    SEND_KBD_REQUEST(kbdio, kbdie);
	    
	} /* if (wakeupsigs & kbdsig) */
	
	if (wakeupsigs & gpdsig)
	{
	    GetMsg(gpdmp); /* Only one message */
	    if (gpdio->io_Error != 0)
	    	continue;
	    
#define MOUSE_QUALIFIERS (IEQUALIFIER_LEFTBUTTON | IEQUALIFIER_RBUTTON | \
			  IEQUALIFIER_MIDBUTTON)
		    
	    InputDevice->ActQualifier &= ~MOUSE_QUALIFIERS;
	    InputDevice->ActQualifier |= (gpdie->ie_Qualifier & MOUSE_QUALIFIERS);
	    
	    /* Gameport just returns the frame count since the last
	       report in ie_TimeStamp.tv_secs; we therefore must add
	       a real timestamp ourselves */
	    GetSysTime(&gpdie->ie_TimeStamp);	    
	    
	    /* Wheel events come in as IECLASS_NEWMOUSE, so fix ie_Class and ie_Qualifier) */
	    if (gpdie->ie_Class == IECLASS_NEWMOUSE)
	    {
	    	#warning "The NewMouse standard seems to send both a IECLASS_NEWMOUSE and a IECLASS_RAWKEY event"
	    	gpdie->ie_Class     = IECLASS_RAWKEY;
	    	gpdie->ie_Qualifier = InputDevice->ActQualifier & KEY_QUALIFIERS;
	    }
	    
	    /* Add event to queue */
	    AddEQTail(gpdie, InputDevice);

	    /* New event from gameport device */
    	    D(bug("id: Gameport event\n"));
	    D(bug("id: Forwarding events\n"));

	    /* Forward event (and possible others in the queue) */
	    ForwardQueuedEvents(InputDevice);
    	    D(bug("id: Events forwarded\n"));
	    
	    /* Wit for some more events */
	    SEND_GPD_REQUEST(gpdio, gpdie);
	    
	} /* if (wakeupsigs & gpdsig) */
	
	if (wakeupsigs & InputDevice->ResetSig)
	{
	    struct IOStdReq resetio = *kbdio;
	    
	    InputDevice->ResetSig = 0;
	    
    	    ShowImminentReset();
	    
	    resetio.io_Command = KBD_RESETHANDLERDONE;
	    resetio.io_Data = &resethandler;
	    
	    /* Relying on this cmd being done quick, here */
	    
	    DoIO((struct IORequest *)&resetio);
	    
	}
	
    } /* Forever */
   
} /* ProcessEvents */


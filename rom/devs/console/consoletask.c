/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Code executed by the console.device task.
    Lang: english
*/

#include <proto/exec.h>
#include <proto/boopsi.h>
#include <exec/io.h>
#include <exec/alerts.h>
#include <dos/dos.h>


#include <devices/input.h>

#include "console_gcc.h"

/* #define DEBUG 1 */
#include <aros/debug.h>

/* Protos */
static BOOL checkconunit(Object *unit, struct ConsoleBase *ConsoleDevice);

VOID consoleTaskEntry(struct coTaskParams *ctp)
{
    struct ConsoleBase *ConsoleDevice = ctp->consoleDevice;
    
    BOOL success = FALSE;
    LONG waitsigs = 0L, wakeupsig;
    
    
    /* CD input handler puts data into this port */
    struct MsgPort *inputport;
    
    
    /* Used for input.device */
    struct MsgPort *inputmp;
    
    /* Add the CDInputHandler to input.device's list of input handlers */
    inputmp = CreateMsgPort();
    if (inputmp)
    {
    	struct IOStdReq *inputio;
    	inputio = (struct IOStdReq *)CreateIORequest(inputmp, sizeof (struct IOStdReq));
	if (inputio)
	{
	    /* Open the input.device */
	    if (!OpenDevice("input.device", -1, (struct IORequest *)inputio, NULL))
	    {
	    	/* Initialize the inputhandler itself */
	    	ConsoleDevice->inputHandler = initCDIH(ConsoleDevice);
	    	if (ConsoleDevice->inputHandler)
	    	{
		    inputio->io_Data = ConsoleDevice->inputHandler;
		    inputio->io_Command = IND_ADDHANDLER;
		    
		    DoIO((struct IORequest *)inputio);
		    success = TRUE;
		}
		CloseDevice((struct IORequest *)inputio);

	    }
	    
	    DeleteIORequest((struct IORequest *)inputio);
	}
    
    	DeleteMsgPort(inputmp);
    }
    
    /* if not successfull,  throw an alert */
    if (!success)
    {
    	Alert(AT_DeadEnd | AN_ConsoleDev | AG_OpenDev | AO_Unknown);
    }
    
    /* Signal the parent task that we have been initialized */
    Signal(ctp->parentTask, ctp->initSignal);
    
    D(bug("Console task initialized\n"));
    
    
    /* Get console.device input handler's port */
    inputport = ((struct cdihData *)ConsoleDevice->inputHandler->is_Data)->inputPort;
    
    waitsigs |= 1 << inputport->mp_SigBit;
    waitsigs |= SIGBREAKF_CTRL_C;
    
    for (;;)
    {
    	wakeupsig = Wait(waitsigs);
	
	/* Anyone wanting to kill us ? */
	if (wakeupsig & SIGBREAKF_CTRL_C)
	{
	    break;
	    
	}
	else if (wakeupsig & (1 << inputport->mp_SigBit))
	{
	    /* A message from the console device input handler */
	    struct cdihMessage *cdihmsg;
	    while ( (cdihmsg = (struct cdihMessage *)GetMsg(inputport)) )
	    {
	    	/* Handle the message */
		
		D(bug("Got message: %s\n", cdihmsg->inputBuf));
		
		/* Check that the ConUnit has not been disposed,
		   while the message was passed
		*/
		
		if (checkconunit(cdihmsg->unit, ConsoleDevice))
		{
		     /* Copy the input over to the unit's buffer */
		     strcpy(ICU(cdihmsg->unit)->inputBuf, cdihmsg->inputBuf);
		}
		
		/* Tell the input handler we're done */
		ReplyMsg((struct Message *)cdihmsg);
	    } /* while (all pending messages from the CD inputhandler) */
	    
	} 
	   
    } /* forever */
    
#warning FIXME: Do cleanup here
    /* Should do some cleanup here */
    
}

/*********************
**  checkconunit()  **
*********************/

/* Checks that the supplied unit has not been disposed */
static BOOL checkconunit(Object *unit, struct ConsoleBase *ConsoleDevice)
{
    Object *o, *ostate;
    BOOL found = FALSE;
    
    ObtainSemaphoreShared(&ConsoleDevice->unitListLock);
    
    ostate = (Object *)ConsoleDevice->unitList.mlh_Head;
    while ( (o = NextObject(&ostate)) && (!found) )
    {
    	if (o == unit)
	{
	    found = TRUE;
	}
    }
    
    ReleaseSemaphore(&ConsoleDevice->unitListLock);
    return found;
}

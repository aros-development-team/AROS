/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Code executed by the console.device task.
    Lang: english
*/


#define AROS_ALMOST_COMPATIBLE 1

#include <exec/lists.h>

#include <proto/exec.h>
#include <proto/boopsi.h>
#include <exec/io.h>
#include <exec/alerts.h>
#include <dos/dos.h>


#include <devices/input.h>

#include <sys/types.h>
#include <signal.h>

#include "console_gcc.h"

/* #define DEBUG 1 */
#include <aros/debug.h>

/* Protos */
static BOOL checkconunit(Object *unit, struct ConsoleBase *ConsoleDevice);
static void answer_read_request(struct IOStdReq *req, struct ConsoleBase *ConsoleDevice);

VOID consoleTaskEntry(struct coTaskParams *ctp)
{
    struct ConsoleBase *ConsoleDevice = ctp->consoleDevice;
    
    BOOL success = FALSE;
    LONG waitsigs = 0L, wakeupsig, commandsig, inputsig;
    
    
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
	    if (!OpenDevice("input.device", -1, (struct IORequest *)inputio, 0UL))
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
    
    
    if (success)
    {
    	/* Create a messageport that will be used to receive commands */
	ConsoleDevice->commandPort = CreateMsgPort();
	if (NULL == CreateMsgPort())
	    success = FALSE;
    }
    
    NEWLIST(&ConsoleDevice->readRequests);
    
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
    
    inputsig	= 1 << inputport->mp_SigBit;
    commandsig	= 1 << ConsoleDevice->commandPort->mp_SigBit;
    
    waitsigs = inputsig|commandsig|SIGBREAKF_CTRL_C;
    
    for (;;)
    {
    	wakeupsig = Wait(waitsigs);
	
	/* Anyone wanting to kill us ? */
	if (wakeupsig & SIGBREAKF_CTRL_C)
	{
	    break;
	    
	}
	else if (wakeupsig & inputsig)
	{
	    /* A message from the console device input handler */
	    struct cdihMessage *cdihmsg;
	    while ( (cdihmsg = (struct cdihMessage *)GetMsg(inputport)) )

	    {

kprintf("GOT INPUT FROM CONSOLE INPUT HANDLER: ");
printstring(cdihmsg->inputBuf, cdihmsg->numBytes, ConsoleDevice);
		/* Check that the ConUnit has not been disposed,
		   while the message was passed
		*/
		if (checkconunit(cdihmsg->unit, ConsoleDevice))
		{
		    struct IOStdReq *req;
		    ULONG tocopy;
		    
		    /* Echo the newly received characters to the console */
		    writeToConsole((struct ConUnit *)cdihmsg->unit
		    	, cdihmsg->inputBuf
			, cdihmsg->numBytes
			, ConsoleDevice
		    );
		    
		    /* Copy received characters to the console unit input buffer.
		       If the buffer is full, then console input will be lost
		    */
		    tocopy = MIN(cdihmsg->numBytes, CON_INPUTBUF_SIZE - ICU(cdihmsg->unit)->numStoredChars);
		    /* Copy the input over to the unit's buffer */
		    CopyMem(cdihmsg->inputBuf
		     	, ICU(cdihmsg->unit)->inputBuf + ICU(cdihmsg->unit)->numStoredChars
			, tocopy
		    );
		    
		    ICU(cdihmsg->unit)->numStoredChars += tocopy;
		    
		    /* See if there are any queued io read requests that wants to be replied */
		    ForeachNode(&ConsoleDevice->readRequests, req)
		    {

		     	if ((APTR)req->io_Unit == (APTR)cdihmsg->unit)
			{
			    /* Paranoia */
			    if (0 != ICU(req->io_Unit)->numStoredChars)
			    {
			     	answer_read_request(req, ConsoleDevice);
			    }
			}
		    }
		}
		
		/* Tell the input handler we're done */
		ReplyMsg((struct Message *)cdihmsg);
	    }
	    
	}
	else if (wakeupsig & commandsig)
	{
	    /* We got a command from the outside. Investigate it */
	    struct IOStdReq *req;
	    
	    
	    while ((req = (struct IOStdReq *)GetMsg(ConsoleDevice->commandPort)))
	    {
	    	switch (req->io_Command)
		{
		    case CMD_READ:
			if (0 != ICU(req->io_Unit)->numStoredChars)
			{
			    answer_read_request(req, ConsoleDevice);
			}
			else
			{
			    /* Not enough bytes in the buffer to fill the request, put it on hold */
			    
			    /* ioReq allready removed from the queue qith GetMsg() */
			    
			    AddTail((struct List *)&ConsoleDevice->readRequests, (struct Node *)req);
			}
			break;
		
		default:
			kprintf("!!! UNKNOWN COMMAND RECEIVED BY CONSOLE TASK !!!\n");
			kprintf("!!! THIS SHOULD NEVER HAPPEN !!!\n");
			break;
		}
	    }
	}
	   
    } /* forever */
    
#warning FIXME: Do cleanup here
    
}

/********** checkconunit()  *******************************/

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

/******** answer_read_request() ***************************/

static void answer_read_request(struct IOStdReq *req, struct ConsoleBase *ConsoleDevice)
{
    Object *unit;
    
    kprintf("answer_read_request\n");
    /* This function assumes that there are at least one character
       available in the unitsinput buffer 
    */

    unit = (Object *)req->io_Unit;

    req->io_Actual = MIN(ICU(unit)->numStoredChars, req->io_Length);

    /* Copy characters from the buffer into the request */
    CopyMem( (APTR)ICU(unit)->inputBuf
    	    , req->io_Data
    	    , req->io_Actual
    );

    if (ICU(unit)->numStoredChars > req->io_Length)
    {
    	ULONG i;
    	
    	ICU(unit)->numStoredChars -= req->io_Actual;
       
    	/* We have to move the rest of the bytes to the start
    	   of the buffer.
    	   
    	   NOTE: we could alternatively use a circular buffer
    	*/
    	
    	for (i = 0; i < ICU(unit)->numStoredChars; i ++)
    	{
    	    ICU(unit)->inputBuf[i] = ICU(unit)->inputBuf[i] + req->io_Actual;
    	}
    }
    else
    {
    	/* No more unread characters in the buffer */
    	ICU(unit)->numStoredChars = 0;
    	
    }
    
    req->io_Error = 0;
    /* All done. Just reply the request */

    Remove((struct Node *)req);

   
/* kprintf("receiving task=%s, sigbit=%d\n, mode=%d"
	, ((struct Task *)req->io_Message.mn_ReplyPort->mp_SigTask)->tc_Node.ln_Name
	, req->io_Message.mn_ReplyPort->mp_SigBit
	, req->io_Message.mn_ReplyPort->mp_Flags
);

    req->io_Flags |= IOF_QUICK;
*/    ReplyMsg((struct Message *)req);
    return;
}

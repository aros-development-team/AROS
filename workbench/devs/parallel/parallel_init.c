/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parallel device
    Lang: English
*/

/****************************************************************************************/


#include <string.h>

#include <exec/resident.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/initializers.h>
#include <devices/parallel.h>
#include <devices/newstyle.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/input.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <oop/oop.h>
#include <hidd/parallel.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#    include "parallel_intern.h"
#endif

#define DEBUG 0
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

/****************************************************************************************/

#define NEWSTYLE_DEVICE 1

struct parallelbase * pubParallelBase;

/****************************************************************************************/

#if NEWSTYLE_DEVICE

static const UWORD SupportedCommands[] =
{
    CMD_READ,
    CMD_WRITE,
    CMD_CLEAR,
    CMD_RESET,
    CMD_FLUSH,
    PDCMD_QUERY,
    PDCMD_SETPARAMS,
    NSCMD_DEVICEQUERY,
    0
};

#endif

/****************************************************************************************/

AROS_SET_LIBFUNC(GM_UNIQUENAME(Init), LIBBASETYPE, ParallelDevice)
{
  AROS_SET_LIBFUNC_INIT

  D(bug("parallel device: init\n"));

  pubParallelBase = ParallelDevice;

  /* open the parallel hidd */
  if (NULL == ParallelDevice->ParallelHidd)
  {
    ParallelDevice->ParallelHidd = OpenLibrary("DRIVERS:parallel.hidd",0);
    D(bug("parallel.hidd base: 0x%x\n",ParallelDevice->ParallelHidd));
    
    if (NULL == ParallelDevice->ParallelHidd)
    	return FALSE;
	
    if (NULL == ParallelDevice->oopBase)
      ParallelDevice->oopBase = OpenLibrary(AROSOOP_NAME, 0);    
    if (NULL == ParallelDevice->oopBase)
    {
    	CloseLibrary(ParallelDevice->ParallelHidd);
	ParallelDevice->ParallelHidd = NULL;
    	return FALSE;
    }
	
    ParallelDevice->ParallelObject = OOP_NewObject(NULL, CLID_Hidd_Parallel, NULL);
	
    if (NULL == ParallelDevice->ParallelObject)
    {
    	CloseLibrary(ParallelDevice->oopBase);
	ParallelDevice->oopBase = NULL;
	CloseLibrary(ParallelDevice->ParallelHidd);
	ParallelDevice->ParallelHidd = NULL;
      	return FALSE;
    }
  }
    
  NEWLIST(&ParallelDevice->UnitList);
  return TRUE;

  AROS_SET_LIBFUNC_EXIT
}


/****************************************************************************************/

AROS_SET_OPENDEVFUNC(GM_UNIQUENAME(Open),
		     LIBBASETYPE, ParallelDevice,
		     struct IORequest, ioreq,
		     unitnum,
		     flags
)
{
  AROS_SET_DEVFUNC_INIT

  struct ParallelUnit * PU = NULL;

  D(bug("parallel device: Open unit %d\n",unitnum));

  if (ioreq->io_Message.mn_Length < sizeof(struct IOExtPar))
  {
      D(bug("parallel.device/open: IORequest structure passed to OpenDevice is too small!\n"));
      ioreq->io_Error = IOERR_OPENFAIL;
      return FALSE;
  }

  ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

  /* In the list of available units look for the one with the same 
     UnitNumber as the given one  */
  if (0 == ioreq->io_Error)
  {
    PU = findUnit(ParallelDevice, unitnum);

    /* If there is no such unit, yet, then create it */
    if (NULL == PU)
    {
      D(bug("Creating Unit %d\n",unitnum));
      PU = AllocMem(sizeof(struct ParallelUnit), MEMF_CLEAR|MEMF_PUBLIC);
      if (NULL != PU)
      {
        PU->pu_OpenerCount	= 1;
        PU->pu_UnitNum		= unitnum;
        PU->pu_Flags		= ioreq->io_Flags;
        
        /*
        ** Initialize the message ports
        */
        NEWLIST(&PU->pu_QReadCommandPort.mp_MsgList);
        PU->pu_QReadCommandPort.mp_Node.ln_Type = NT_MSGPORT;
          
        NEWLIST(&PU->pu_QWriteCommandPort.mp_MsgList);
        PU->pu_QWriteCommandPort.mp_Node.ln_Type= NT_MSGPORT;
   
        InitSemaphore(&PU->pu_Lock);
        /* do further initilization here. Like getting the ParallelUnit Object etc. */

        PU->pu_Unit  = HIDD_Parallel_NewUnit(ParallelDevice->ParallelObject, unitnum);
        if (NULL != PU->pu_Unit)
        {
          HIDD_ParallelUnit_Init(PU->pu_Unit, RBF_InterruptHandler, NULL, WBE_InterruptHandler, NULL);
          ioreq->io_Device = (struct Device *)ParallelDevice;
          ioreq->io_Unit   = (struct Unit *)PU;  

          /*
          ** put it in the list of open units
          */
          AddHead(&ParallelDevice->UnitList, (struct Node *)PU);

          ioreq->io_Error  = 0;
 
          return TRUE;
        }

        D(bug("ParallelUnit could not be created!\n"));
          
        FreeMem(PU, sizeof(struct ParallelUnit));

        ioreq->io_Error = ParErr_DevBusy;
      }
    }
    else
    {
      /* the unit does already exist. */
      /* 
      ** Check whether one more opener to this unit is tolerated 
      */
      if (0 != (PU->pu_Flags & PARF_SHARED))
      {
        /*
        ** This unit is in shared mode and one more opener
        ** won't hurt.
        */
        ioreq->io_Device = (struct Device *)ParallelDevice;
        ioreq->io_Unit   = (struct Unit *)PU;
        ioreq->io_Error  = 0;

        PU->pu_OpenerCount++;
      }
      else
      {
        /*
        ** I don't allow another opener
        */
        ioreq->io_Error = ParErr_DevBusy;
      }
    }
  }

  return TRUE;
     
  AROS_SET_DEVFUNC_EXIT
}


/****************************************************************************************/

AROS_SET_CLOSEDEVFUNC(GM_UNIQUENAME(Close),
		      LIBBASETYPE, ParallelDevice,
		      struct IORequest, ioreq
)
{
  AROS_SET_DEVFUNC_INIT
  struct ParallelUnit * PU = (struct ParallelUnit *)ioreq->io_Unit;

  /*
  ** Check whether I am the last opener to this unit
  */
  if (1 == PU->pu_OpenerCount)
  {
    /*
    ** I was the last opener. So let's get rid of it.
    */
    /*
    ** Remove the unit from the list
    */
    Remove((struct Node *)&PU->pu_Node);
    
    HIDD_Parallel_DisposeUnit(ParallelDevice->ParallelObject, PU->pu_Unit);
  
    FreeMem(PU, sizeof(struct ParallelUnit));
  
  }
  else
  {
    /*
    ** There are still openers. Decrease the counter.
    */
    PU->pu_OpenerCount--;
  }

  return TRUE;

  AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_LIBFUNC(GM_UNIQUENAME(Expunge), LIBBASETYPE, ParallelDevice)
{
    AROS_SET_LIBFUNC_INIT

    if (NULL != ParallelDevice->ParallelObject)
    {
      /*
      ** Throw away the HIDD object and close the library
      */
      OOP_DisposeObject(ParallelDevice->ParallelObject);
      CloseLibrary(ParallelDevice->ParallelHidd);
      ParallelDevice->ParallelHidd   = NULL;
      ParallelDevice->ParallelObject = NULL;
    }
    if (ParallelDevice->oopBase) CloseLibrary(ParallelDevice->oopBase);
    
    return TRUE;

    AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

/****************************************************************************************/

#define ioStd(x)  ((struct IOStdReq *)x)
AROS_LH1(void, beginio,
 AROS_LHA(struct IOExtPar *, ioreq, A1),
	   struct parallelbase *, ParallelDevice, 5, Parallel)
{
  AROS_LIBFUNC_INIT
  
  struct ParallelUnit * PU = (struct ParallelUnit *)ioreq->IOPar.io_Unit;

  D(bug("parallel device: beginio(ioreq=%p)\n", ioreq));

  /* WaitIO will look into this */
  ioreq->IOPar.io_Message.mn_Node.ln_Type=NT_MESSAGE;

  /* 
  ** As a lot of "public" data can be modified in the following lines
  ** I protect it from other tasks by this semaphore 
  */  
  ObtainSemaphore(&PU->pu_Lock);

  switch (ioreq->IOPar.io_Command)
  {
#if NEWSTYLE_DEVICE
  case NSCMD_DEVICEQUERY:
    if(ioreq->IOPar.io_Length < ((LONG)OFFSET(NSDeviceQueryResult, SupportedCommands)) + sizeof(UWORD *))
    {
      ioreq->IOPar.io_Error = IOERR_BADLENGTH;
    }
    else
    {
      struct NSDeviceQueryResult *d;

      d = (struct NSDeviceQueryResult *)ioreq->IOPar.io_Data;

      d->DevQueryFormat 	= 0;
      d->SizeAvailable 	 	= sizeof(struct NSDeviceQueryResult);
      d->DeviceType 	 	= NSDEVTYPE_PARALLEL;
      d->DeviceSubType 	 	= 0;
      d->SupportedCommands 	= (UWORD *)SupportedCommands;

      ioreq->IOPar.io_Actual = sizeof(struct NSDeviceQueryResult);
      ioreq->IOPar.io_Error  = 0;

      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOPar.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOPar.io_Message);
    }
    break;
#endif
 
    /*******************************************************************/
    case CMD_READ:
      /*
      **  Let me see whether I can copy any data at all and
      **  whether nobody else is using this device now
       */
      ioreq->IOPar.io_Actual = 0;

      Disable();

      PU->pu_Status |= STATUS_READS_PENDING;
      D(bug("Queuing the read request.\n"));
      /*
      **  Everything that falls down here could not be completely
      **  satisfied
      */
      if (NULL == PU->pu_ActiveRead)
        PU->pu_ActiveRead = &ioreq->IOPar.io_Message;
      else
        PutMsg(&PU->pu_QReadCommandPort,
               &ioreq->IOPar.io_Message);

      Enable();
      /*
      ** As I am returning immediately I will tell that this
      ** could not be done QUICK   
      */
      ioreq->IOPar.io_Flags &= ~IOF_QUICK;
    break;

    /*******************************************************************/

    case CMD_WRITE:
      /* Write data to the ParallelUnit */
      ioreq->IOPar.io_Actual = 0;
  
      Disable();

      /* Check whether I can write some data immediately */
      if (0 == (PU->pu_Status & STATUS_WRITES_PENDING))
      {
        ULONG writtenbytes;
        BOOL complete = FALSE;
        /* 
           Writing the first few bytes to the UART has to have the
           effect that whenever the UART can receive new data
           a HW interrupt must happen. So this writing to the
           UART should get the sequence of HW-interrupts going
           until there is no more data to write 
	*/
	if (-1 == ioreq->IOPar.io_Length)
	{
	  int stringlen = strlen(ioreq->IOPar.io_Data);
	  D(bug("Transmitting NULL termninated string.\n"));
	  /*
	  ** Supposed to write the buffer to the port until a '\0'
	  ** is encountered.
	  */
	  
	  writtenbytes = HIDD_ParallelUnit_Write(PU->pu_Unit,
	                                         ioreq->IOPar.io_Data,
	                                         stringlen);
	  if (writtenbytes == stringlen)
	    complete = TRUE;
	  else
	    PU->pu_WriteLength = stringlen-writtenbytes;
	}
	else
	{
          writtenbytes = HIDD_ParallelUnit_Write(PU->pu_Unit,
                                                 ioreq->IOPar.io_Data,
                                                 ioreq->IOPar.io_Length);
          if (writtenbytes == ioreq->IOPar.io_Length)
            complete = TRUE;
          else
	    PU->pu_WriteLength = ioreq->IOPar.io_Length-writtenbytes;
        }
        /*
        ** A consistency check between the STATUS_WRITES_PENDING flag
        ** and the pointer PU->pu_ActiveWrite which both have to be
        ** set or cleared at the same time.
        */
        if (NULL != PU->pu_ActiveWrite)
        {
          D(bug("error!!"));
        }

        if (complete == TRUE)
        {
          D(bug("completely sended the stream!\n"));
          /*
          ** The request could be completed immediately.
          ** Check if I have to reply the message
          */
          if (0 == (ioreq->IOPar.io_Flags & IOF_QUICK))
            ReplyMsg(&ioreq->IOPar.io_Message);
        }
        else
        {
          /*
          ** The request could not be completed immediately
          ** Clear the flag.
          */
          ioreq->IOPar.io_Flags &= ~IOF_QUICK;
          PU->pu_ActiveWrite = (struct Message *)ioreq;
          PU->pu_Status |= STATUS_WRITES_PENDING;
	  PU->pu_NextToWrite = writtenbytes;
        }
      }
      else
      {    
        /* 
           I could not write the data immediately as another request
           is already there. So I will make this
           the responsibility of the interrupt handler to use this
           request once it is done with the active request.
        */
        PutMsg(&PU->pu_QWriteCommandPort,
               (struct Message *)ioreq);
	PU->pu_Status |= STATUS_WRITES_PENDING;
        /* 
        ** As I am returning immediately I will tell that this
        ** could not be done QUICK   
        */
        ioreq->IOPar.io_Flags &= ~IOF_QUICK;
      }
      
      Enable();
    break;

    case CMD_CLEAR:
      /* Simply reset the input buffer pointer no matter what */
      ioreq->IOPar.io_Error = 0;    
      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOPar.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOPar.io_Message);
    break;

    /*******************************************************************/

    case CMD_RESET:
      Disable();
      /* All IORequests, including the active ones, are aborted */

      /* Abort the active IORequests */
      PU->pu_Status &= ~(STATUS_READS_PENDING|STATUS_WRITES_PENDING);

      if (NULL != PU->pu_ActiveRead)
      {
        ((struct IOStdReq *)PU->pu_ActiveRead)->io_Error = IOERR_ABORTED;
        ReplyMsg(PU->pu_ActiveRead);
      }

      if (NULL != PU->pu_ActiveWrite)
      {
        ((struct IOStdReq *)PU->pu_ActiveWrite)->io_Error = IOERR_ABORTED;
        ReplyMsg(PU->pu_ActiveWrite);
      }
      Enable();

    /*******************************************************************/

    case CMD_FLUSH:
      /* 
      ** Clear all queued IO request for the given parallel unit except
      ** for the active ones.
       */
      Disable();
      
      while (TRUE)
      {
        struct IOStdReq * iopreq = 
                  (struct IOStdReq *)GetMsg(&PU->pu_QReadCommandPort);
        if (NULL == iopreq)
          break;
        iopreq->io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *)iopreq);        
      }

      while (TRUE)
      {
        struct IOStdReq * iopreq = 
                  (struct IOStdReq *)GetMsg(&PU->pu_QWriteCommandPort);
        if (NULL == iopreq)
          break;
        iopreq->io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *)iopreq);        
      }
      ioreq->IOPar.io_Error = 0;

      Enable();
      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOPar.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOPar.io_Message);
    break;

    /*******************************************************************/

    case CMD_START:
    break;

    /*******************************************************************/
    
    case CMD_STOP:
    break;

    /*******************************************************************/

    case PDCMD_QUERY:

      PU->pu_Status = 0;

      /*
      ** set the io_Status to the status of the parallel port
      */
      // !!! missing code
      ioreq->io_Status = 0;

      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOPar.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOPar.io_Message);

    break;

    /*******************************************************************/

    case PDCMD_SETPARAMS:
        
      /* Change of buffer size for input buffer? */


      /* Copy the Flags from the iorequest to the Unit's Flags */
      PU->pu_Flags = ioreq->io_ParFlags;

      /* Copy the TermArray */
      PU->pu_PTermArray = ioreq->io_PTermArray;
        
      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOPar.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOPar.io_Message);
    break;

    /*******************************************************************/

    default:
      /* unknown command */
      ioreq->IOPar.io_Error = IOERR_NOCMD;
      
      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOPar.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOPar.io_Message);
    
  } /* switch () */

  ReleaseSemaphore(&PU->pu_Lock);
  
  D(bug("id: Return from BeginIO()\n"));

  AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct parallelbase *, ParallelDevice, 6, Parallel)
{
  AROS_LIBFUNC_INIT

  struct ParallelUnit  * PU = (struct ParallelUnit *)ioreq->io_Unit;

  /*
  ** is it the active request?
  */
  
  Disable();
  if ((struct Message *)ioreq == PU->pu_ActiveRead)
  {
    /*
    ** It's the active reuquest. I make the next available
    ** one the active request.
    */
    PU->pu_ActiveRead = GetMsg(&PU->pu_QReadCommandPort);
    ReplyMsg(&ioreq->io_Message);
  }
  else
  {
    /*
    ** It's not the active request. So I'll take it out of the
    ** list of queued messages and reply the message.
    */
    Remove(&ioreq->io_Message.mn_Node);
    ReplyMsg(&ioreq->io_Message);
  }
  Enable();
  
  return 0;
  AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

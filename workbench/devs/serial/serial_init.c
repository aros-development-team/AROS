/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial device
    Lang: english
*/

/****************************************************************************************/


#include <string.h>

#include <exec/resident.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/initializers.h>
#include <devices/serial.h>
#include <devices/newstyle.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/input.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <oop/oop.h>
#include <hidd/serial.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/lists.h>
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#    include "serial_intern.h"
#endif

# define DEBUG 1
# include <aros/debug.h>

/****************************************************************************************/

#define NEWSTYLE_DEVICE 1

/****************************************************************************************/

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct serialbase *AROS_SLIB_ENTRY(init,Serial)();
void AROS_SLIB_ENTRY(open,Serial)();
BPTR AROS_SLIB_ENTRY(close,Serial)();
BPTR AROS_SLIB_ENTRY(expunge,Serial)();
int AROS_SLIB_ENTRY(null,Serial)();
void AROS_SLIB_ENTRY(beginio,Serial)();
LONG AROS_SLIB_ENTRY(abortio,Serial)();

static const char end;

/****************************************************************************************/

int AROS_SLIB_ENTRY(entry,Serial)(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

/****************************************************************************************/

const struct Resident Serial_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Serial_resident,
    (APTR)&end,
    RTF_AUTOINIT|RTF_COLDSTART,
    41,
    NT_DEVICE,
    30,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]=SERIALNAME;

static const char version[]="$VER: serial 41.0 (2.28.1999)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct serialbase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,Serial)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,Serial),
    &AROS_SLIB_ENTRY(close,Serial),
    &AROS_SLIB_ENTRY(expunge,Serial),
    &AROS_SLIB_ENTRY(null,Serial),
    &AROS_SLIB_ENTRY(beginio,Serial),
    &AROS_SLIB_ENTRY(abortio,Serial),
    (void *)-1
};

struct ExecBase * SysBase;

struct serialbase * pubSerialBase;

/****************************************************************************************/

#if NEWSTYLE_DEVICE

static const UWORD SupportedCommands[] =
{
    CMD_READ,
    CMD_WRITE,
    CMD_CLEAR,
    CMD_RESET,
    CMD_FLUSH,
    SDCMD_QUERY,
    SDCMD_SETPARAMS,
    SDCMD_BREAK,    
    NSCMD_DEVICEQUERY,
    0
};

#endif

/****************************************************************************************/

AROS_UFH3(struct serialbase *, AROS_SLIB_ENTRY(init,Serial),
 AROS_UFHA(struct serialbase *, SerialDevice, D0),
 AROS_UFHA(BPTR               , segList     , A0),
 AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
  AROS_USERFUNC_INIT

  SysBase = sysBase;

  D(bug("serial device: init\n"));

  pubSerialBase = SerialDevice;

  /* Store arguments */
  SerialDevice->sysBase = sysBase;
  SerialDevice->seglist = segList;
    
  /* open the serial hidd */
  if (NULL == SerialDevice->SerialHidd)
  {
    SerialDevice->SerialHidd = OpenLibrary("DRIVERS:serial.hidd",0);
    D(bug("serial.hidd base: 0x%x\n",SerialDevice->SerialHidd));
    if (NULL == SerialDevice->SerialHidd)
	return NULL;
    
    if (NULL == SerialDevice->oopBase)
      SerialDevice->oopBase = OpenLibrary(AROSOOP_NAME, 0);
    if (NULL == SerialDevice->oopBase)
    {
	CloseLibrary(SerialDevice->SerialHidd);
	SerialDevice->SerialHidd = NULL;
	return NULL;
    }

    SerialDevice->SerialObject = OOP_NewObject(NULL, CLID_Hidd_Serial, NULL);
    D(bug("serialHidd Object: 0x%x\n",SerialDevice->SerialObject));
    if (NULL == SerialDevice->SerialObject)
    {
	CloseLibrary(SerialDevice->oopBase);
	SerialDevice->oopBase = NULL;
	CloseLibrary(SerialDevice->SerialHidd);
	SerialDevice->SerialHidd = NULL;
	return NULL;
    }
  }
    
  NEWLIST(&SerialDevice->UnitList);
  return (SerialDevice);
  AROS_USERFUNC_EXIT
}


/****************************************************************************************/


AROS_LH3(void, open,
 AROS_LHA(struct IORequest *, ioreq, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	 struct serialbase *, SerialDevice, 1, Serial)
{
  AROS_LIBFUNC_INIT

  struct SerialUnit * SU = NULL;

  D(bug("serial device: Open unit %d\n",unitnum));

  if (ioreq->io_Message.mn_Length < sizeof(struct IOExtSer))
  {
      D(bug("serial.device/open: IORequest structure passed to OpenDevice is too small!\n"));
      ioreq->io_Error = IOERR_OPENFAIL;
      return;
  }

  ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

  /* I have one more opener. */

  /* In the list of available units look for the one with the same 
     UnitNumber as the given one  */
  if (0 == ioreq->io_Error)
  {
    SU = findUnit(SerialDevice, unitnum);

    /* If there is no such unit, yet, then create it */
    if (NULL == SU)
    {
      D(bug("Creating Unit %d\n",unitnum));
      SU = AllocMem(sizeof(struct SerialUnit), MEMF_CLEAR|MEMF_PUBLIC);
      if (NULL != SU)
      {
        SU->su_OpenerCount	= 1;
        SU->su_UnitNum		= unitnum;
        SU->su_SerFlags		= ((struct IOExtSer *)ioreq)->io_SerFlags;
        SU->su_InputBuffer	= AllocMem(MINBUFSIZE, MEMF_PUBLIC);
        
        if (NULL != SU->su_InputBuffer)
        {
          SU->su_InBufLength = MINBUFSIZE;
          ((struct IOExtSer *)ioreq)->io_RBufLen  = MINBUFSIZE;
          
          /*
          ** Initialize the message ports
          */
          NEWLIST(&SU->su_QReadCommandPort.mp_MsgList);
          SU->su_QReadCommandPort.mp_Node.ln_Type = NT_MSGPORT;
          
          NEWLIST(&SU->su_QWriteCommandPort.mp_MsgList);
          SU->su_QWriteCommandPort.mp_Node.ln_Type= NT_MSGPORT;
   
          InitSemaphore(&SU->su_Lock);
          /* do further initilization here. Like getting the SerialUnit Object etc. */

          SU->su_Unit  = HIDD_Serial_NewUnit(SerialDevice->SerialObject, unitnum);
          if (NULL != SU->su_Unit)
          {
            HIDD_SerialUnit_Init(SU->su_Unit, RBF_InterruptHandler, NULL, 
                                              WBE_InterruptHandler, NULL);
            ioreq->io_Device = (struct Device *)SerialDevice;
            ioreq->io_Unit   = (struct Unit *)SU;  
            SerialDevice->device.dd_Library.lib_OpenCnt ++;
            SerialDevice->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

            /*
            ** put it in the list of open units
            */
            AddHead(&SerialDevice->UnitList, (struct Node *)SU);

            ioreq->io_Error  = 0;
            return;
          }

          D(bug("SerialUnit could not be created!\n"));
          ioreq->io_Error = SerErr_DevBusy;
          
          FreeMem(SU->su_InputBuffer, MINBUFSIZE);
        }
        
        FreeMem(SU, sizeof(struct SerialUnit));

        ioreq->io_Error = SerErr_DevBusy;
      }
    }
    else
    {
      /* the unit does already exist. */
      /* 
      ** Check whether one more opener to this unit is tolerated 
      */
      if (0 != (SU->su_SerFlags & SERF_SHARED))
      {
        /*
        ** This unit is in shared mode and one more opener
        ** won't hurt.
        */
        ioreq->io_Device = (struct Device *)SerialDevice;
        ioreq->io_Unit   = (struct Unit *)SU;
        ioreq->io_Error  = 0;

        SU->su_OpenerCount++;
      }
      else
      {
        /*
        ** I don't allow another opener
        */
        ioreq->io_Error = SerErr_DevBusy;
      }
    }
  }
  
  if (ioreq->io_Error == 0)
  {
      SerialDevice->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;
      SerialDevice->device.dd_Library.lib_OpenCnt ++;    
  }
  
  return;
     

  AROS_LIBFUNC_EXIT
}

/****************************************************************************************/


AROS_LH1(BPTR, close,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct serialbase *, SerialDevice, 2, Serial)
{
  AROS_LIBFUNC_INIT
  struct SerialUnit * SU = (struct SerialUnit *)ioreq->io_Unit;

  /*
  ** Check whether I am the last opener to this unit
  */
  if (1 == SU->su_OpenerCount)
  {
    /*
    ** I was the last opener. So let's get rid of it.
    */
    /*
    ** Remove the unit from the list
    */
    Remove((struct Node *)&SU->su_Node);
    
    HIDD_Serial_DisposeUnit(SerialDevice->SerialObject, SU->su_Unit);
  
    if (NULL != SU->su_InputBuffer && 0 != SU->su_InBufLength)
    {
      FreeMem(SU->su_InputBuffer, 
              SU->su_InBufLength);
    }
  
    FreeMem(SU, sizeof(struct SerialUnit));
  
  }
  else
  {
    /*
    ** There are still openers. Decrease the counter.
    */
    SU->su_OpenerCount--;
  }

  /* Let any following attemps to use the device crash hard. */
  ioreq->io_Device=(struct Device *)-1;
  
  SerialDevice->device.dd_Library.lib_OpenCnt --;    
  
  if (SerialDevice->device.dd_Library.lib_OpenCnt == 0)
  {
    if (SerialDevice->device.dd_Library.lib_Flags & LIBF_DELEXP)
    {
    	#define expunge() \
    	    AROS_LC0(BPTR, expunge, struct serialbase *, SerialDevice, 3, Serial)

      return expunge();
    }
  }
  
  return 0;
  AROS_LIBFUNC_EXIT
}

/****************************************************************************************/


AROS_LH0(BPTR, expunge, struct serialbase *, SerialDevice, 3, Serial)
{
    AROS_LIBFUNC_INIT

    BPTR ret = 0;

    if (SerialDevice->device.dd_Library.lib_OpenCnt)
    {
    	SerialDevice->device.dd_Library.lib_Flags |= LIBF_DELEXP;
	return 0;
    }
    
    if (NULL != SerialDevice->SerialObject)
    {
      /*
      ** Throw away the HIDD object and close the library
      */
      OOP_DisposeObject(SerialDevice->SerialObject);
      CloseLibrary(SerialDevice->oopBase);
      CloseLibrary(SerialDevice->SerialHidd);
      SerialDevice->SerialHidd   = NULL;
      SerialDevice->oopBase      = NULL;
      SerialDevice->SerialObject = NULL;
    }
  
    Remove(&SerialDevice->device.dd_Library.lib_Node);
    
    ret = SerialDevice->seglist;
    
    FreeMem((char *)SerialDevice - SerialDevice->device.dd_Library.lib_NegSize,
    	    SerialDevice->device.dd_Library.lib_NegSize +
	    SerialDevice->device.dd_Library.lib_PosSize);
    
    return ret;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/


AROS_LH0I(int, null, 
   struct serialbase *, SerialDevice, 4, Serial)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

#define ioStd(x)  ((struct IOStdReq *)x)
AROS_LH1(void, beginio,
 AROS_LHA(struct IOExtSer *, ioreq, A1),
	   struct serialbase *, SerialDevice, 5, Serial)
{
  AROS_LIBFUNC_INIT
  
  BOOL success;
  struct SerialUnit * SU = (struct SerialUnit *)ioreq->IOSer.io_Unit;

  D(bug("serial device: beginio(ioreq=%p)\n", ioreq));

  /* WaitIO will look into this */
  ioreq->IOSer.io_Message.mn_Node.ln_Type=NT_MESSAGE;

  /* 
  ** As a lot of "public" data can be modified in the following lines
  ** I protect it from other tasks by this semaphore 
  */  
  ObtainSemaphore(&SU->su_Lock);

  switch (ioreq->IOSer.io_Command)
  {
#if NEWSTYLE_DEVICE
  case NSCMD_DEVICEQUERY:
    if(ioreq->IOSer.io_Length < ((LONG)OFFSET(NSDeviceQueryResult, SupportedCommands)) + sizeof(UWORD *))
    {
      ioreq->IOSer.io_Error = IOERR_BADLENGTH;
    }
    else
    {
      struct NSDeviceQueryResult *d;

      d = (struct NSDeviceQueryResult *)ioreq->IOSer.io_Data;

      d->DevQueryFormat 	= 0;
      d->SizeAvailable 	 	= sizeof(struct NSDeviceQueryResult);
      d->DeviceType 	 	= NSDEVTYPE_SERIAL;
      d->DeviceSubType 	 	= 0;
      d->SupportedCommands 	= (UWORD *)SupportedCommands;

      ioreq->IOSer.io_Actual = sizeof(struct NSDeviceQueryResult);
      ioreq->IOSer.io_Error  = 0;

      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOSer.io_Message);
    }
    break;
#endif
 
    /*******************************************************************/
    case CMD_READ:
      /*
      **  Let me see whether I can copy any data at all and
      **  whether nobody else is using this device now
       */
      ioreq->IOSer.io_Actual = 0;      

      Disable();

      if (SU->su_InputFirst != SU->su_InputNextPos &&
          0 == (SU->su_Status & STATUS_READS_PENDING)    )
      {
        D(bug("There are data in the receive buffer!\n"));
        /* 
           No matter how many bytes are in the input buffer,
           I will copy them into the IORequest buffer immediately.
           This way I can satisfy requests that are larger than
           the input buffer, because the Interrupt will have
           to put its bytes directly into the buffer of the
           request from now on.
         */
        if (-1 == ioreq->IOSer.io_Length)
	{
          if (TRUE == copyInDataUntilZero(SU, (struct IOStdReq *)ioreq))
	  {
            /* 
            ** it could be completed immediately.
            ** Check if I have to reply the message.
            */
            if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
              ReplyMsg(&ioreq->IOSer.io_Message);

            Enable();
            break;
	  }
        }
        else
	{
          D(bug("Calling copyInData!\n"));
          if (TRUE == copyInData(SU, (struct IOStdReq *)ioreq))
	  {
            /* 
            ** it could be completed immediately.
            ** Check if I have to reply the message.
            */
            if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
              ReplyMsg(&ioreq->IOSer.io_Message);

            Enable();
            break;
	  } 
	}

        if (NULL != SU->su_ActiveRead)
        {
          kprintf("READ: error in datastructure!");
        }

        
      }

      D(bug("Setting STATUS_READS_PENDING\n"));
  
      SU->su_Status |= STATUS_READS_PENDING;

      D(bug("The read request (%p) could not be satisfied! Queuing it.\n",ioreq));
      /*
      **  Everything that falls down here could not be completely
      **  satisfied
      */
      if (NULL != SU->su_ActiveRead) {
        SU->su_ActiveRead = (struct Message *)ioreq;
      } else {
        PutMsg(&SU->su_QReadCommandPort,
               (struct Message *)ioreq);
      }

      Enable();
     
      /*
      ** As I am returning immediately I will tell that this
      ** could not be done QUICK   
      */
      ioreq->IOSer.io_Flags &= ~IOF_QUICK;
    break;

    /*******************************************************************/

    case CMD_WRITE:
      /* Write data to the SerialUnit */
      ioreq->IOSer.io_Actual = 0;

      Disable();

      /* Check whether I can write some data immediately */
      if (0 == (SU->su_Status & STATUS_WRITES_PENDING))
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
	if (-1 == ioreq->IOSer.io_Length)
	{
	  int stringlen = strlen(ioreq->IOSer.io_Data);
	  D(bug("Transmitting NULL termninated string.\n"));
	  /*
	  ** Supposed to write the buffer to the port until a '\0'
	  ** is encountered.
	  */
	  
	  writtenbytes = HIDD_SerialUnit_Write(SU->su_Unit,
	                                       ioreq->IOSer.io_Data,
	                                       stringlen);
	  if (writtenbytes == stringlen)
	    complete = TRUE;
	  else
	    SU->su_WriteLength = stringlen-writtenbytes;
	}
	else
	{
          writtenbytes = HIDD_SerialUnit_Write(SU->su_Unit,
                                               ioreq->IOSer.io_Data,
                                               ioreq->IOSer.io_Length);
          if (writtenbytes == ioreq->IOSer.io_Length)
            complete = TRUE;
	  else
	    SU->su_WriteLength = ioreq->IOSer.io_Length-writtenbytes;
        }
        /*
        ** A consistency check between the STATUS_WRITES_PENDING flag
        ** and the pointer SU->su_ActiveWrite which both have to be
        ** set or cleared at the same time.
        */
        if (NULL != SU->su_ActiveWrite)
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
          if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
            ReplyMsg(&ioreq->IOSer.io_Message);
        }
        else
        {
          /*
          ** The request could not be completed immediately
          ** Clear the flag.
          */
          ioreq->IOSer.io_Flags &= ~IOF_QUICK;
          SU->su_ActiveWrite = (struct Message *)ioreq;
          SU->su_Status |= STATUS_WRITES_PENDING; 
          SU->su_NextToWrite = writtenbytes;
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
        PutMsg(&SU->su_QWriteCommandPort,
               (struct Message *)ioreq);
        SU->su_Status |= STATUS_WRITES_PENDING;
        /* 
        ** As I am returning immediately I will tell that this
        ** could not be done QUICK   
        */
        ioreq->IOSer.io_Flags &= ~IOF_QUICK;
      }

      Enable();

    break;

    case CMD_CLEAR:
      /* Simply reset the input buffer pointer no matter what */
      Disable();
      SU->su_InputNextPos = 0;
      SU->su_InputFirst = 0;
      SU->su_Status &= ~STATUS_BUFFEROVERFLOW;
      ioreq->IOSer.io_Error = 0;    
      Enable();
      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOSer.io_Message);
    break;

    /*******************************************************************/

    case CMD_RESET:
      Disable();
      /* All IORequests, including the active ones, are aborted */
      /* Abort the active IORequests */
      SU->su_Status &= ~(STATUS_READS_PENDING|STATUS_WRITES_PENDING);

      if (NULL != SU->su_ActiveRead)
      {
        ((struct IOStdReq *)SU->su_ActiveRead)->io_Error = IOERR_ABORTED;
        ReplyMsg(SU->su_ActiveRead);
      }

      if (NULL != SU->su_ActiveWrite)
      {
        ((struct IOStdReq *)SU->su_ActiveWrite)->io_Error = IOERR_ABORTED;
        ReplyMsg(SU->su_ActiveWrite);
      }

      /* change the Buffer pointers to reset position */
      SU->su_InputFirst   = 0;
      SU->su_InputNextPos = 0;

      /* check the buffer for correct init size */
      if (MINBUFSIZE != SU->su_InBufLength)
      {
        BYTE * oldBuffer = SU->su_InputBuffer;
        BYTE * newBuffer = (BYTE *)AllocMem(MINBUFSIZE ,MEMF_PUBLIC);
        if (NULL != newBuffer)
	{
          SU->su_InputBuffer = newBuffer;
          FreeMem(oldBuffer, SU->su_InBufLength);
          /* write new buffersize */
          SU->su_InBufLength = MINBUFSIZE;
        }
        else
	{
          /* Buffer could not be allocated */
	}
      }
      Enable();
      /* now fall through to CMD_FLUSH */
      

    /*******************************************************************/

    case CMD_FLUSH:
      /* 
      ** Clear all queued IO request for the given serial unit except
      ** for the active ones.
       */
      Disable();

      while (TRUE)
      {
        struct IOStdReq * iosreq = 
                  (struct IOStdReq *)GetMsg(&SU->su_QReadCommandPort);
        if (NULL == iosreq)
          break;
        iosreq->io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *)iosreq);        
      }

      while (TRUE)
      {
        struct IOStdReq * iosreq = 
                  (struct IOStdReq *)GetMsg(&SU->su_QWriteCommandPort);
        if (NULL == iosreq)
          break;
        iosreq->io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *)iosreq);        
      }
      ioreq->IOSer.io_Error = 0;

      Enable();

      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOSer.io_Message);
    break;

    /*******************************************************************/

    case CMD_START:
      /*
       * Start the serial port IO. Tell the hidd to do that.
       */
      Disable();
      HIDD_SerialUnit_Start(SU->su_Unit);
      Enable();
      if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOSer.io_Message);
      ioreq->IOSer.io_Flags |= IOF_QUICK;
    break;

    /*******************************************************************/
    case CMD_STOP:
      /*
       * Stop any serial port IO going on. Tell the hidd to do that.
       */
      Disable();
      HIDD_SerialUnit_Stop(SU->su_Unit);
      Enable();
      if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOSer.io_Message);
      ioreq->IOSer.io_Flags |= IOF_QUICK;
    break;

    /*******************************************************************/

    case SDCMD_QUERY:
      Disable();
      /*
      ** set the io_Status to the status of the serial port
      */
      ioreq->io_Status = HIDD_SerialUnit_GetStatus(SU->su_Unit);
      if (0 != (SU->su_Status & STATUS_BUFFEROVERFLOW))
      {
        ioreq->io_Status |= IO_STATF_OVERRUN;
        ioreq->IOSer.io_Actual = 0;
      }
      else
      {
        /* pass back the number of unread input characters */
        int unread = SU->su_InputNextPos - SU->su_InputFirst;
        if (unread < 0)
          ioreq->IOSer.io_Actual = SU->su_InBufLength + unread;
        else
          ioreq->IOSer.io_Actual = unread;
      }

      ioreq->IOSer.io_Error = 0; 
      Enable();
      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOSer.io_Message);

    break;

    /*******************************************************************/

    case SDCMD_SETPARAMS:
        
      /* Change of buffer size for input buffer? */
      Disable();

      if (ioreq->io_RBufLen >= MINBUFSIZE &&
          ioreq->io_RBufLen != SU->su_InBufLength)
      {
        /* 
        ** The other ones I will only do if I am not busy with
        ** reading or writing data right now 
        */
        if (0 == (SU->su_Status & (STATUS_READS_PENDING |
                                   STATUS_WRITES_PENDING) ))
	{
          /* the user requests a different inputbuffer size.
             he shall have it. */
          int OldInBufLength = SU->su_InBufLength;
          BYTE * OldInBuf    = SU->su_InputBuffer;
          
          BYTE * NewInBuf;
          NewInBuf = AllocMem(ioreq->io_RBufLen, MEMF_PUBLIC);
          if (NULL != NewInBuf)
          {
            /* we got a new buffer */
            /* just in case the interrupt wants to write data to
               the input buffer, tell it that it cannot do this right
               now as I am changing the buffer */
            SU->su_Status      |= STATUS_CHANGING_IN_BUFFER;
            SU->su_InputNextPos = 0;
            SU->su_InputFirst   = 0;
            SU->su_InputBuffer  = NewInBuf;
            SU->su_InBufLength  = ioreq->io_RBufLen;
            SU->su_Status      &= ~STATUS_CHANGING_IN_BUFFER;
           
            /* free the old buffer */ 
            FreeMem(OldInBuf, OldInBufLength); 
          }
          else
          {
            ioreq->IOSer.io_Error = SerErr_BufErr;
            return;
          }
          /* end of buffer changing buiseness */
        }
      }
        
      /* Changing the break time */        
      if (ioreq->io_BrkTime != 0)
        SU->su_BrkTime = ioreq->io_BrkTime;
        
      /* Copy the Flags from the iorequest to the Unit's Flags */
      SU->su_SerFlags = ioreq->io_SerFlags;

      /* Change baudrate if necessary and possible */
      if (SU->su_Baud != ioreq->io_Baud)
      {
        /* Change the Baudrate */
        D(bug("Setting baudrate on SerialUnit!\n"));
        success = HIDD_SerialUnit_SetBaudrate(SU->su_Unit, 
                                              ioreq->io_Baud);

        if (FALSE == success)
        {
          D(bug("Setting baudrate didn't work!\n"));
          /* this Baudrate is not supported */
          ioreq->IOSer.io_Error = SerErr_BaudMismatch;
          return; 
        }
        SU->su_Baud = ioreq->io_Baud;
      } /* Baudrate changing */
        
      /* Copy the TermArray */
      SU->su_TermArray = ioreq->io_TermArray;
        
      /* copy the readlen and writelen */
      if (SU->su_ReadLen != ioreq->io_ReadLen ||
          SU->su_WriteLen != ioreq->io_WriteLen ||
          SU->su_StopBits != ioreq->io_StopBits)
      {
        struct TagItem tags[] = 
          {{TAG_DATALENGTH, ioreq->io_ReadLen},
           {TAG_STOP_BITS , ioreq->io_StopBits},
           {TAG_SKIP      , 0},  // !!! PARITY!!!
           {TAG_END       , 0}};
        success = HIDD_SerialUnit_SetParameters(SU->su_Unit,
                                                tags);
        if (FALSE == success) {
          ioreq->IOSer.io_Error = SerErr_InvParam;
          kprintf("HIDD_SerialUnit_SetParameters() failed.\n");
          return;
	}
        SU->su_ReadLen  = ioreq->io_ReadLen;
        SU->su_WriteLen = ioreq->io_WriteLen;
        SU->su_StopBits = ioreq->io_StopBits;
      }
      
      SU->su_CtlChar  = ioreq->io_CtlChar;
      Enable();

      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOSer.io_Message);
    break;

    /*******************************************************************/

    case SDCMD_BREAK:
      Disable();
      if (0 != (ioreq->io_SerFlags & SERF_QUEUEDBRK))
      {
        /* might have to queue that request */
        if (0 != (SU->su_Status & STATUS_WRITES_PENDING))
        {
kprintf("%s: Queuing SDCMD_BREAK! This probably doesn't work correctly!\n");
          PutMsg(&SU->su_QWriteCommandPort,
                 (struct Message *)ioreq);
          ioreq->IOSer.io_Flags &= ~IOF_QUICK;
          break;
        }
      }
      /* Immediately execute this command */
      ioreq->IOSer.io_Error = HIDD_SerialUnit_SendBreak(SU->su_Unit, 
                                                        SU->su_BrkTime);
      Enable();

      if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOSer.io_Message);
      
    break;

    /*******************************************************************/

    default:
      /* unknown command */
      ioreq->IOSer.io_Error = IOERR_NOCMD;
      
      /*
      ** The request could be completed immediately.
      ** Check if I have to reply the message
      */
      if (0 == (ioreq->IOSer.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOSer.io_Message);
      
    
  } /* switch () */

  ReleaseSemaphore(&SU->su_Lock);  
  
  D(bug("id: Return from BeginIO()\n"));

  AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct serialbase *, SerialDevice, 6, Serial)
{
  AROS_LIBFUNC_INIT

  struct SerialUnit  * SU = (struct SerialUnit *)ioreq->io_Unit;

  /*
  ** is it the active request?
  */

  Disable();
  if ((struct Message *)ioreq == SU->su_ActiveRead)
  {
    /*
    ** It's the active reuquest. I make the next available
    ** one the active request.
    */
    SU->su_ActiveRead = GetMsg(&SU->su_QReadCommandPort);
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

static const char end=0;

/****************************************************************************************/

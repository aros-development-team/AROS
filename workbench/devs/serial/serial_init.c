/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Serial device
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <exec/resident.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <devices/serial.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/input.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <aros/libcall.h>
#ifdef __GNUC__
#    include "serial_intern.h"
#endif

#define DEBUG 0
#include <aros/debug.h>

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


int AROS_SLIB_ENTRY(entry,Serial)(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

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

static const char name[]="serial.device";

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

AROS_LH2(struct serialbase *, init,
 AROS_LHA(struct serialbase *, SerialDevice, D0),
 AROS_LHA(BPTR               , segList     , A0),
	   struct ExecBase *, sysBase, 0, Serial)
{
    AROS_LIBFUNC_INIT

    //kprintf("serial device: init\n");

    /* Store arguments */
    SerialDevice->sysBase = sysBase;
    SerialDevice->seglist = segList;
    
    SysBase = sysBase;
    
    SerialDevice->device.dd_Library.lib_OpenCnt=1;

    return (SerialDevice);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, open,
 AROS_LHA(struct IORequest *, ioreq, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	 struct serialbase *, SerialDevice, 1, Serial)
{
  AROS_LIBFUNC_INIT

  struct SerialUnit * SU = SerialDevice->FirstUnit;

  kprintf("serial device: Open unit %d\n",unitnum);

  /* Keep compiler happy */
    
  ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

  /* I have one more opener. */
  SerialDevice->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;
  SerialDevice->device.dd_Library.lib_OpenCnt ++;

  /* In the list of available units look for the one with the same 
     UnitNumber as the given one  */
  if (0 == ioreq->io_Error)
  {
    while (NULL != SU)
    {
      if (SU->su_UnitNum == unitnum)
        break;
      SU = SU->su_Next;
    }
  
    /* If there is no such unit, yet, then create it */
    if (NULL == SU)
    {
      kprintf("Creating Unit %d\n",unitnum);
      SU = AllocMem(sizeof(struct SerialUnit), MEMF_CLEAR|MEMF_PUBLIC);
      if (NULL != SU)
      {
        SU -> su_InputBuffer = AllocMem(MINBUFSIZE, MEMF_PUBLIC);
        
        if (NULL != SU->su_InputBuffer)
        {
          SU->su_InBufLength = MINBUFSIZE;
          
          SU -> su_Next    = SerialDevice->FirstUnit;
          SerialDevice->FirstUnit = SU;

          SU -> su_UnitNum = unitnum;
          SU -> su_Unit.unit_OpenCnt = 1;
   
          InitSemaphore(&SU->su_Lock);
          /* do further initilization here. Like getting the HIDD etc. */

          /*
          ** SU -> HIDD = ...
          */
        }
        else
        {
          /* Didn't get an input buffer */
          FreeMem(SU, sizeof(struct SerialUnit));
          ioreq->io_Error = SerErr_DevBusy;
        }
      }
      else
      {
        kprintf("error: Couldn't create unit!\n");
        ioreq->io_Error = SerErr_DevBusy;
      }
    }
    else
    {
      /* the unit does already exist. */
      /* Check whether one more opener to this unit is tolerated */
     
    }
  }
  
  ioreq->io_Device = (struct Device *)SerialDevice;
  ioreq->io_Unit   = (struct Unit *)SU;  
  

  if (0 != ioreq->io_Error)
  {
      SerialDevice->device.dd_Library.lib_OpenCnt ++;
      SerialDevice->device.dd_Library.lib_Flags&=~LIBF_DELEXP;
      return;
  }
  
  return;
     

  AROS_LIBFUNC_EXIT
}



AROS_LH1(BPTR, close,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct serialbase *, SerialDevice, 2, Serial)
{
    AROS_LIBFUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    ioreq->io_Device=(struct Device *)-1;
    return 0;
    AROS_LIBFUNC_EXIT
}



AROS_LH0(BPTR, expunge, struct serialbase *, SerialDevice, 3, Serial)
{
    AROS_LIBFUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    SerialDevice->device.dd_Library.lib_Flags|=LIBF_DELEXP;
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, 
   struct serialbase *, SerialDevice, 4, Serial)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

#define ioStd(x)  ((struct IOStdReq *)x)
AROS_LH1(void, beginio,
 AROS_LHA(struct IOExtSer *, ioreq, A1),
	   struct serialbase *, SerialDevice, 5, Serial)
{
  AROS_LIBFUNC_INIT
  
  BOOL success;
  struct SerialUnit * SU = (struct SerialUnit *)ioreq->IOSer.io_Unit;

  kprintf("serial device: beginio(ioreq=%p)\n", ioreq);

  /* WaitIO will look into this */
  ioreq->IOSer.io_Message.mn_Node.ln_Type=NT_MESSAGE;

  /* 
  ** As a lot of "public" data can be modified in the following lines
  ** I protect it from other tasks by this semaphore 
  */  
  ObtainSemaphore(&SU->su_Lock);

  switch (ioreq->IOSer.io_Command)
  {
    case CMD_READ:
      /*
      **  Let me see whether I can copy any data at all and
      **  whether nobody else is using this device now
       */
      ioreq->IOSer.io_Actual = 0;      

      if (SU->su_InputFirst != SU->su_InputNextPos &&
          0 == (SU->su_Status & STATUS_READS_PENDING)    )
      {
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
            /* it could be satisfied completely */
            ioreq->IOSer.io_Flags |= IOF_QUICK;
            break;
	  }
        }
        else
	{
          if (TRUE == copyInData(SU, (struct IOStdReq *)ioreq))
	  {
            /* it could be satisfied completely */
            ioreq->IOSer.io_Flags |= IOF_QUICK;
            break;
	  } 
	}

        if (NULL != SU->su_ActiveRead)
          kprintf("READ: error in datastructure!");
        SU->su_ActiveRead = (struct Message *)ioreq;
        SU->su_Status |= STATUS_READS_PENDING;
        break;
      }
      /*
      **  Everything that falls down here could not be completely
      **  satisfied
      */

      PutMsg((struct MsgPort *)&SU->su_QReadCommandPort,
             (struct Message *)ioreq);
      /*
      ** As I am returning immediately I will tell that this
      ** could not be done QUICK   
       */
      ioreq->IOSer.io_Flags &= ~IOF_QUICK;
    break;

    /*******************************************************************/

    case CMD_WRITE:
      /* Write data to the UART */
      ioreq->IOSer.io_Actual = 0;      

      /* Check whether I can write some data immediately */
      if (0 == (SU->su_Status & STATUS_WRITES_PENDING))
      {
        int writtenbytes;
        /* I can write the first (few) byte(s) immediately */
        SU->su_Status |= STATUS_WRITES_PENDING;
        /* Writing the first few bytes to the UART has to have the
           effect that whenever the UART can receive new data
           a HW interrupt must happen. So this writing to the
           UART should get the sequence of HW-interrupts going
           until there is no more data to write 
	*/
        writtenbytes = 0;
        // HIDD_WRITE_BYTES(SU->su_Hidd, (struct IOStdReq *)ioreq);
        if (NULL != SU->su_ActiveWrite)
          kprintf("error!!");
        SU->su_ActiveWrite = (struct Message *)ioreq;
        SU->su_Status |= STATUS_WRITES_PENDING;
        break;
      }    
      /* I could not write the data immediately as another request
         is already there. So I will make this
         the responsibility of the interrupt handler to use this
         request once it is done with the active request.
      */
      PutMsg((struct MsgPort *)&SU->su_QWriteCommandPort,
             (struct Message *)ioreq);
    break;

    case CMD_CLEAR:
      /* Simply reset the input buffer pointer no matter what */
      SU->su_InputNextPos = 0;
      SU->su_InputFirst = 0;
      ioreq->IOSer.io_Error = 0;      
    break;

    /*******************************************************************/

    case CMD_RESET:
      /* All IOrequests, including the active ones, are aborted */

      /* Abort the active IORequests */
      SU->su_Status &= ~(STATUS_READS_PENDING|STATUS_WRITES_PENDING);
      if (NULL != SU->su_ActiveRead)
      {
        /* do I have to leave anything in the message ? */
        ReplyMsg(SU->su_ActiveRead);
      }

      if (NULL != SU->su_ActiveWrite)
      {
        /* do I have to leave anything in the message ? */
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
          /* Buffer could not be allocated*/
	}
      }
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
        /* What do I have to leave in the request to tell the user
           that the request was not satisfied?? Anyhting at all? */
        ReplyMsg((struct Message *)iosreq);        
      }

      while (TRUE)
      {
        struct IOStdReq * iosreq = 
                  (struct IOStdReq *)GetMsg(&SU->su_QWriteCommandPort);
        if (NULL == iosreq)
          break;
        /* What do I have to leave in the request to tell the user
           that the request was not satisfied?? Anyhting at all? */
        ReplyMsg((struct Message *)iosreq);        
      }
      ioreq->IOSer.io_Error = 0;

      Enable();
    break;

    /*******************************************************************/

    case SDCMD_QUERY:

      SU->su_Status = 0;
      // SU->su_Status = HIDD_QUERY(SU->su_HIDD); 
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
          ioreq->IOSer.io_Actual = -unread;
        else
          ioreq->IOSer.io_Actual = SU->su_InBufLength - unread;
      }

    break;

    /*******************************************************************/

    case SDCMD_SETPARAMS:
        
      /* Change of buffer size for input buffer? */
      
      if (ioreq->io_RBufLen >= MINBUFSIZE &&
          ioreq->io_RBufLen != SU->su_InBufLength)
      {
         /* the other ones I will only do if I am not busy with
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
            SU->su_Status     |= STATUS_CHANGING_IN_BUFFER;
            SU->su_InputNextPos = 0;
            SU->su_InputFirst   = 0;
            SU->su_InputBuffer = NewInBuf;
            SU->su_InBufLength = ioreq->io_RBufLen;
            SU->su_Status     &= ~STATUS_CHANGING_IN_BUFFER;
           
            /* free the old buffer */ 
            FreeMem(OldInBuf, OldInBufLength); 
          }
          else
          {
            ioreq->IOSer.io_Error = SerErr_BufErr;
            return;
          }
          /* end of buffer changing buiseness */

        

          /* Changing the break time */        
          if (ioreq->io_BrkTime != 0)
            SU->su_BrkTime = ioreq->io_BrkTime;
        
          /* Copy the Flags from the iorequest to the Unit's Flags */
          SU->su_Flags = ioreq->io_SerFlags;
        
         
          /* the device doesn't seem busy right now, so 
          ** I will make the other changes
          */
          
          /* Change baudrate if necessary and possible */
          if (SU->su_Baud != ioreq->io_Baud)
          {
            /* Change the Baudrate */
            success = TRUE ; // just for now
            //success = SERHIDD_CHANGE_BAUDRATE(SU->su_HIDD, SU->su_Baud);
            if (FALSE == success)
            {
              /* this Baudrate is not supported */
              ioreq->IOSer.io_Error = SerErr_BaudMismatch;
              return; 
            } 
            SU->su_Baud = ioreq->io_Baud;
          } /* Baudrate changing */
        
          /* Copy the TermArray */
          SU->su_TermArray = ioreq->io_TermArray;
        
          /* copy the readlen and writelen */
          if (SU->su_ReadLen != ioreq->io_ReadLen)
	  {
            success = TRUE; // for now
            // success = SERHIDD_SET_READLEN(SU->su_Hidd, ioreq->io_ReadLen);
            if (FALSE == success)
	    {
              ioreq->IOSer.io_Error = SerErr_InvParam;
              return;   
	    } 
            SU->su_ReadLen  = ioreq->io_ReadLen;
	  }
          if (SU->su_WriteLen != ioreq->io_WriteLen)
	  {
            success = TRUE; // for now
            // success = SERHIDD_SET_WRITELEN(SU->su_Hidd, ioreq->io_ReadLen);
            if (FALSE == success)
	    {
              ioreq->IOSer.io_Error = SerErr_InvParam;
              return;   
	    } 
            SU->su_WriteLen  = ioreq->io_WriteLen;
	  }

          if (SU->su_StopBits != ioreq->io_StopBits)
	  {
            success = TRUE; // for now
            // success = SERHIDD_SET_READLEN(SU->su_Hidd, ioreq->io_ReadLen);
            if (FALSE == success)
	    {
              ioreq->IOSer.io_Error = SerErr_InvParam;
              return;   
	    } 
            SU->su_StopBits  = ioreq->io_StopBits;
	  }

          SU->su_CtlChar  = ioreq->io_CtlChar;
	}
      }        
    break;

    /*******************************************************************/

    
  } /* switch () */

  ReleaseSemaphore(&SU->su_Lock);  
  
  //kprinf("id: Return from BeginIO()\n");

  AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct serialbase *, SerialDevice, 6, Serial)
{
    AROS_LIBFUNC_INIT

    /* TODO!! */

    return 0;
    AROS_LIBFUNC_EXIT
}

static const char end=0;

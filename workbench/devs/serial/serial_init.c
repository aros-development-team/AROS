/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Serial device
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <exec/resident.h>
#include <exec/interrupts.h>
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
    
  /* If the Serial task does not exist yet, then this is our
     first customer...
  */
  if (NULL == SerialDevice->SerialTask)
  {
    /* Since the serial device task will use this before this function
    ** has exited, we can put the following structure on the stack
    */
    struct IDTaskParams idtask_params;
    idtask_params.SerialDevice = SerialDevice;
    idtask_params.Caller = FindTask(NULL);
        
    /* We don't use a OS signel (like SIGBREAKF_CTRL_D), because
    ** we might get it from elsewere.
    */
    idtask_params.Signal = 1 << ioreq->io_Message.mn_ReplyPort->mp_SigBit;
   	
    kprintf("serial device: Creating serial task\n");
    	    
    SerialDevice->SerialTask = CreateSerialTask(&idtask_params, SerialDevice);
    	    
    kprintf("serial device: serial task created: %p\n", SerialDevice->SerialTask);
    	
    if (SerialDevice->SerialTask)
    {
      /* Here we wait for the serial task to initialize it's
      ** command msgport etc. (see processevents.c). This
      ** is to prevent race conditions.
      ** Say that we exited succesfully now and did
      ** an asynchronous IO request to the device, while
      ** the serial.device yet not had created it's command port.
      ** It would most certainly crash the machine
      */
      kprintf("serial device: Waiting for serial task to initialize itself\n");
      Wait (idtask_params.Signal);
      kprintf("serial device: Got signal from serial task\n");
    	    	
      ioreq->io_Error = NULL;

      /* I have one more opener. */
      SerialDevice->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;
      SerialDevice->device.dd_Library.lib_OpenCnt ++;
    } /* if (input task created) */
    else
    {
      /* no serial task could be created */
      /* It seems like nothing could go wrong on real Amigas as all
         the necessary Errors are deactivated in serial.h. Oh, well
         the user just will learn that the device is busy */
      ioreq->io_Error = SerErr_DevBusy;  
    }
  } /* if (first time opened) */

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

  switch (ioreq->IOSer.io_Command)
  {
    case CMD_READ:
      PutMsg((struct MsgPort *)&SerialDevice->CommandPort, 
             (struct Message *)ioreq);
      /* pretending there were not enough data immediately */
      ioreq->IOSer.io_Flags &= ~IOF_QUICK;
    break;
   
    case CMD_CLEAR:
      /* Simply reset the input buffer pointer no matter what */
      SU->su_InputCurPos = 0;
      ioreq->IOSer.io_Error = 0;      
    break;
  
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
            SU->su_InputCurPos = 0;
            SU->su_InputBuffer = NewInBuf;
            SU->su_InBufLength = ioreq->io_RBufLen;
            SU->su_Status     &= ~STATUS_CHANGING_IN_BUFFER;
           
            /* free the old buffer */ 
            FreeMem(OldInBuf, OldInBufLength); 
          }
          else
          {
            ioreq->io_Error = SerErr_BufErr;
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
  } /* switch () */
    
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

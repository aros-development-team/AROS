#include <aros/asmcall.h>
#include <aros/libcall.h>
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <exec/interrupts.h>
#include <hidd/serial.h>
#include "serial_intern.h"

#define DEBUG 0
#include <aros/debug.h>

extern struct serialbase * pubSerialBase;

ULONG RBF_InterruptHandler(UBYTE * data, ULONG length, ULONG unitnum)
{
  struct SerialUnit * SU = pubSerialBase->FirstUnit;
  ULONG index = 0;

  D(bug("!Received %d bytes on unit %d (%s)\n",length,unitnum,data));

  while (NULL != SU)
  {
    if (SU->su_UnitNum == unitnum)
      break;
    SU = SU->su_Next;
  }
  
  if (NULL != SU)
  {
    if (0 != (SU->su_Status & STATUS_READS_PENDING))
    {
      struct IOStdReq * ioreq;
      ioreq = (struct IOStdReq *)SU->su_ActiveRead;

      if (NULL == ioreq)
      {
        ioreq = (struct IOStdReq *)GetMsg(&SU->su_QReadCommandPort);
        SU->su_ActiveRead = (struct Message *)ioreq;
        D(bug("Something is wrong!"));
      }
      
      while (NULL != ioreq)
      {
        /*
        ** Copy the remaining data into a request buffer.
        ** This loop woll possibly execute several times
        */
        UBYTE * destBuf;
        UWORD indexDestBuf;
        D(bug("Have a IORequest for Serial device!\n"));
        
        destBuf = ioreq->io_Data;
        indexDestBuf = ioreq->io_Actual;
        /*
        ** I copy as many bytes as I can into this request
        */
        while (index < length)
        {
          destBuf[indexDestBuf] = data[index];
            
          index++;
          indexDestBuf++;

          D(bug("io_Length %d:  io_Actual: %d\n",ioreq->io_Length,indexDestBuf));

          if ((-1 == ioreq->io_Length && 0 == destBuf[indexDestBuf-1]) ||
              (indexDestBuf == ioreq->io_Length))
          {
            /*
            ** this request is done, I answer the message
            */
            ioreq->io_Actual = indexDestBuf;
            ReplyMsg((struct Message *)ioreq);
              
            /*
            ** Get the next request ...
            */
            ioreq = (struct IOStdReq *)GetMsg(&SU->su_QReadCommandPort);
            SU->su_ActiveRead = (struct Message *)ioreq;
            break;    
          }
        }
        
        if (index == length && NULL != ioreq)
        {
          ioreq->io_Actual = indexDestBuf;
          break;
        }
      }
      if (NULL == ioreq)
        SU->su_Status &= ~STATUS_READS_PENDING;
      
    }
  } /* if (NULL != su) */   

  if (index < length)
  {
    /*
    ** there's no more IORequest, so I have to copy into the
    ** genearal buffer
    */
 
    D(bug("Copying data into general buffer\n"));

    SU->su_Status &= ~STATUS_READS_PENDING;
    while (index < length)
    {
      if (0 == (SU->su_Status & STATUS_BUFFEROVERFLOW))
      {
        UWORD tmp = (SU->su_InputNextPos + 1) % SU->su_InBufLength;
        SU->su_InputBuffer[SU->su_InputNextPos] = data[index];
        index++;
            
        /*
        ** I am advancing the circular index su_InputNextPos
        */
        if (tmp != SU->su_InputFirst)
        {  
          SU->su_InputNextPos = tmp;
        }
        else
        {
          SU->su_Status |= STATUS_BUFFEROVERFLOW;
          break;
        }
            
      }
    }
  }
  
  return length;
}


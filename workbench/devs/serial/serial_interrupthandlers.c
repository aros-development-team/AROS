#include <aros/asmcall.h>
#include <aros/libcall.h>
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <exec/interrupts.h>
#include "serial_intern.h"

/****************************************************************************
  The RBF interrupthandler is called whenever there is a byte/
  are bytes on the serial port to be picked up.
****************************************************************************/

AROS_UFH1(void, RBF_InterruptHandler,
  AROS_UFHA(struct serialbase *, sb, A1))
{
  BYTE * UARTBuffer;
  UWORD index;
  UWORD length;
  struct SerialUnit * SU = sb->FirstUnit;

  BYTE DummyInput = 0x12;
  
  /* There's one problem with this interrupthandler:
     It is only meant for one UART and not for more. So 
     in this routine it will have to handle all of the
     installed and used UARTs.
   */
  while(NULL != SU)
  {
    /* The following function should read at least one byte from the
       serial port and pass it to this function */
  
    UARTBuffer = &DummyInput;
    length = 1;
    index = 0;
    /* The buffer in the HIDD MUST be preallocated as nobody ever
       may allocated memory in a interrupt!!!
    */
    // UARTBuffer = HIDD_READ_UART_BYTEARRAY(SU->su_Hidd,&length);
    
    /* Now let me see what I have to do with these data ... */
    if (0 != (SU->su_Status & STATUS_READS_PENDING))
    {
      struct IOStdReq * ioreq;
      while (TRUE)
      {
        /*  I can copy the (remaining) data into a request buffer.
        **  This loop will possibly be executed several times 
         */
        BYTE * outBuf;
        UWORD indexOutBuf;
        ioreq = (struct IOStdReq *)SU->su_ReadCommandPort.mp_MsgList.lh_Head;
        if (NULL == ioreq)
        {
          break;
        }
        
        outBuf = ioreq->io_Data;
        indexOutBuf = ioreq->io_Actual;
        /* I copy as many bytes as I can into this request */
        if (-1 == ioreq->io_Length)
        {
          /* Copy until a 0 comes up */
          while (index < length)
          {
            outBuf[indexOutBuf] = UARTBuffer[index];
            
            if (0 == outBuf[indexOutBuf])
            {
              /* this request is done, so I answer the message */
              struct Message * msg = GetMsg(&SU->su_ReadCommandPort);

              /* the final 0 has been found */
              ioreq->io_Actual = indexOutBuf;

              ReplyMsg(msg);
              break;
            }
            indexOutBuf++;
            index ++;
          }
          
          if (index == length)
            break;
        }
      }
      
      if (NULL == ioreq)
      {
        /* there's no more IORequest waiting, so I fill the buffer with
           the remaining data */
        /*
          There are no more reads pending
        */
        SU->su_Status &= ~STATUS_READS_PENDING;
        while (index < length)
        {
          /* Check whether the buffer is not full, yet */
          if (0 == (SU->su_Status & STATUS_BUFFEROVERFLOW ))    
          {
            UWORD tmp = (SU->su_InputNextPos + 1) % SU->su_InBufLength;
            SU->su_InputBuffer[SU->su_InputNextPos] = UARTBuffer[index];
            index++;
            
            /* andvance the circular index su_InputNextPos */
            if (tmp != SU->su_InputFirst)
              SU->su_InputNextPos = tmp;
            else
            {
              /* Buffer full !!! */
              SU->su_Status |= STATUS_BUFFEROVERFLOW;
              break;
            }    
          }
          else
          {
            /* Buffer full!!! */
            SU->status |= STATUS_BUFFEROVERFLOW;
            break;
          }
        }
      }
    }
    SU = SU->su_Next;
  }
}


AROS_UFH1(void, TBE_InterruptHandler,
  AROS_UFHA(struct serialbase *, sb, A1))
{
 
}
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <aros/libcall.h>
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <exec/interrupts.h>
#include <hidd/parallel.h>
#include "parallel_intern.h"

#define DEBUG 0
#include <aros/debug.h>

extern struct parallelbase * pubParallelBase;

ULONG RBF_InterruptHandler(UBYTE * data, ULONG length, ULONG unitnum, APTR userdata)
{
  struct ParallelUnit * PU = NULL;
  ULONG index = 0;

  D(bug("!Received %d bytes on unit %d (%s)\n",length,unitnum,data));

  PU = findUnit(pubParallelBase, unitnum);
    
  if (NULL != PU)
  {
    if (0 != (PU->pu_Status & STATUS_READS_PENDING))
    {
      struct IOStdReq * ioreq;
      ioreq = (struct IOStdReq *)PU->pu_ActiveRead;

      if (NULL == ioreq)
      {
        ioreq = (struct IOStdReq *)GetMsg(&PU->pu_QReadCommandPort);
        PU->pu_ActiveRead = (struct Message *)ioreq;
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
        D(bug("Have a IORequest for Parallel device!\n"));
        
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
            ioreq = (struct IOStdReq *)GetMsg(&PU->pu_QReadCommandPort);
            PU->pu_ActiveRead = (struct Message *)ioreq;
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
        PU->pu_Status &= ~STATUS_READS_PENDING;
      
    }
  } /* if (NULL != pu) */   
 
  /*
  ** Simply dropping the incoming data
  */
  
  return length;
}


/*
 * The write buffer empty interrupt handler
 */
ULONG WBE_InterruptHandler( ULONG unitnum, APTR userdata)
{
	ULONG total = 0;
	struct ParallelUnit * PU;

	PU = findUnit(pubParallelBase, unitnum);

	if (NULL != PU) {
		/*
		 * First get any active write 
		 */
		struct IOExtPar * ioparreq = (struct IOExtPar *)PU->pu_ActiveWrite;
		
		while (1) {
			/*
			 * Try to transmit the active write request
			 */
			if (NULL != ioparreq) {
				ULONG writtenbytes;
				writtenbytes = HIDD_ParallelUnit_Write(PU->pu_Unit,
				                                      &((char *)ioparreq->IOPar.io_Data)[PU->pu_NextToWrite],
				                                      PU->pu_WriteLength);
				/*
				 * Check whether this was written completely.
				 */
				total += writtenbytes;
				if (writtenbytes >= PU->pu_WriteLength) {
					/* This one is done */
					ReplyMsg(&ioparreq->IOPar.io_Message);
				} else {
					/*
					 * Not completed, yet.
					 */
					PU->pu_WriteLength -= writtenbytes;
					PU->pu_NextToWrite += writtenbytes;
					/*
					 * Get out of the loop
					 */
					break;
				}
			}
			/* 
			 * Get the next request from the queue.
			 */
			ioparreq = (struct IOExtPar *)GetMsg(&PU->pu_QWriteCommandPort);
			PU->pu_ActiveWrite = (struct Message *)ioparreq;
			if (NULL == ioparreq) {
				/*
				 * No more request left. Done.
				 */
				PU->pu_Status &= ~STATUS_WRITES_PENDING;
				break;
			}
			
			/*
			 * There is a new request.
			 */
			PU->pu_NextToWrite = 0;
			if (-1 == ioparreq->IOPar.io_Length) {
				PU->pu_WriteLength = strlen(ioparreq->IOPar.io_Data);
			} else {
				PU->pu_WriteLength = ioparreq->IOPar.io_Length;
			}
			/*
			 * And repeat the loop with this request
			 */
		}
	}
	return total;
}

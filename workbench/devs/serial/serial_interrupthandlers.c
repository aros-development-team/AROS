
#include <aros/asmcall.h>
#include <aros/libcall.h>
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <exec/interrupts.h>
#include <hidd/serial.h>
#include <devices/serial.h>
#include <string.h>
#include "serial_intern.h"

#define DEBUG 0
#include <aros/debug.h>

extern struct serialbase * pubSerialBase;

ULONG RBF_InterruptHandler(UBYTE * data, ULONG length, ULONG unitnum, APTR userdata)
{
	struct SerialUnit * SU = NULL;
	ULONG index = 0;

	D(bug("!Received %d bytes on unit %d\n",length,unitnum));

	SU = findUnit(pubSerialBase, unitnum);
    
	if (NULL != SU) {
		if (0 != (SU->su_Status & STATUS_READS_PENDING)) {
			struct IOStdReq * ioreq;
			ioreq = (struct IOStdReq *)SU->su_ActiveRead;

			if (NULL == ioreq) {
				D(bug("\t\tpre  lh_Head: %p\n",SU->su_QReadCommandPort.mp_MsgList.lh_Head));
				ioreq = (struct IOStdReq *)GetMsg(&SU->su_QReadCommandPort);
				D(bug("\t\tpost lh_Head: %p\n",SU->su_QReadCommandPort.mp_MsgList.lh_Head));
				SU->su_ActiveRead = (struct Message *)ioreq;
			}
      
			while (NULL != ioreq) {
				/*
				** Copy the remaining data into a request buffer.
				** This loop woll possibly execute several times
				*/
				UBYTE * destBuf;
				UWORD indexDestBuf;
				D(bug("Have a IORequest (%p) for Serial device!\n",ioreq));
        
				destBuf = ioreq->io_Data;
				indexDestBuf = ioreq->io_Actual;
				/*
				** I copy as many bytes as I can into this request
				*/
				while (index < length) {
					destBuf[indexDestBuf] = data[index];
            
					index++;
					indexDestBuf++;

					D(bug("io_Length %d:  io_Actual: %d\n",ioreq->io_Length,indexDestBuf));

					if ((-1 == ioreq->io_Length && 0 == destBuf[indexDestBuf-1]) ||
					    (indexDestBuf >= ioreq->io_Length)) {
						/*
						** this request is done, I answer the message
						*/
						ioreq->io_Actual = indexDestBuf;
						ReplyMsg((struct Message *)ioreq);
              
						/*
						** Get the next request ...
						*/
						D(bug("\t\tpre  lh_Head: %p\n",SU->su_QReadCommandPort.mp_MsgList.lh_Head));
						ioreq = (struct IOStdReq *)GetMsg(&SU->su_QReadCommandPort);
						D(bug("\t\tpost lh_Head: %p\n",SU->su_QReadCommandPort.mp_MsgList.lh_Head));
						D(bug("\t\tGot new ioreq (%p) from queue\n",ioreq));
						SU->su_ActiveRead = (struct Message *)ioreq;
						break;    
					}
				}
        
				if (index == length && NULL != ioreq) {
					ioreq->io_Actual = indexDestBuf;
					break;
				}
			}
			if (NULL == ioreq)
				SU->su_Status &= ~STATUS_READS_PENDING;
      
		}
	} /* if (NULL != su) */	 

	if (index < length) {
		/*
		** there's no more IORequest, so I have to copy into the
		** genearal buffer
		*/
 
		D(bug("Copying data into general buffer\n"));

		SU->su_Status &= ~STATUS_READS_PENDING;
		while (index < length) {
			if (0 == (SU->su_Status & STATUS_BUFFEROVERFLOW)) {
				UWORD tmp = (SU->su_InputNextPos + 1) % SU->su_InBufLength;
				SU->su_InputBuffer[SU->su_InputNextPos] = data[index];
				index++;

				/*
				** I am advancing the circular index su_InputNextPos
				*/
				if (tmp != SU->su_InputFirst) {	
					SU->su_InputNextPos = tmp;
D(bug("%d %d %d\n",SU->su_InputNextPos,SU->su_InBufLength,tmp));
				} else {
					SU->su_Status |= STATUS_BUFFEROVERFLOW;
					break;
				}
            
			} else 
				break;
		}
	}
	return length;
}

/*
 * The write buffer empty interrupt handler
 */
ULONG WBE_InterruptHandler( ULONG unitnum, APTR userdata)
{
	ULONG total = 0;
	struct SerialUnit * SU;

	SU = findUnit(pubSerialBase, unitnum);

	if (NULL != SU) {
		/*
		 * First get any active write 
		 */
		struct IOExtSer * ioserreq = (struct IOExtSer *)SU->su_ActiveWrite;
		
		while (1) {
			/*
			 * Try to transmit the active write request
			 */
			if (NULL != ioserreq) {
				ULONG writtenbytes;
				writtenbytes = HIDD_SerialUnit_Write(SU->su_Unit,
				                                     &((char *)ioserreq->IOSer.io_Data)[SU->su_NextToWrite],
				                                     SU->su_WriteLength);
				/*
				 * Check whether this was written completely.
				 */
				total += writtenbytes;
				if (writtenbytes >= SU->su_WriteLength) {
					/* This one is done */
					ReplyMsg(&ioserreq->IOSer.io_Message);
				} else {
					/*
					 * Not completed, yet.
					 */
					SU->su_WriteLength -= writtenbytes;
					SU->su_NextToWrite += writtenbytes;
					/*
					 * Get out of the loop
					 */
					break;
				}
			}
			/* 
			 * Get the next request from the queue.
			 */
			ioserreq = (struct IOExtSer *)GetMsg(&SU->su_QWriteCommandPort);
			SU->su_ActiveWrite = (struct Message *)ioserreq;
			if (NULL == ioserreq) {
				/*
				 * No more request left. Done.
				 */
				SU->su_Status &= ~STATUS_WRITES_PENDING;
				break;
			}
			
			/*
			 * There is a new request.
			 */
			SU->su_NextToWrite = 0;
			if (-1 == ioserreq->IOSer.io_Length) {
				SU->su_WriteLength = strlen(ioserreq->IOSer.io_Data);
			} else {
				SU->su_WriteLength = ioserreq->IOSer.io_Length;
			}
			/*
			 * And repeat the loop with this request
			 */
		}
	}
	return total;
}

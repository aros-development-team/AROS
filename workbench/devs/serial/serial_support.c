/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <exec/tasks.h>
#include <exec/lists.h>
#include <exec/memory.h>

#include "serial_intern.h"

#define DEBUG 0
#include <aros/debug.h>

/**************************************************************************
  Copy data from the buffer where it is collected to the destination buffer
  in the IORequest structure
  The return value indicates whether the request could be satisfied 
  completely or not.
**************************************************************************/

BOOL copyInData(struct SerialUnit * SU, struct IOStdReq * ioreq)
{
  UWORD count = 0;
  UWORD index = SU->su_InputFirst;
  UBYTE * Buffer = ioreq->io_Data;
  
  D(bug("su_InputNextPos: %d  su_InputFirst: %d\n",SU->su_InputNextPos, index));
  
  while (count < ioreq->io_Length &&
         SU->su_InputNextPos != index)
  {
    /* copy one byte */
    Buffer[count] = SU->su_InputBuffer[index];

    count++;
    index++;
    /*  
    **  The buffer is organized in a circular fashion with
    **  length SU->su_InBufLength 
     */
    if (index == SU->su_InBufLength)
      index = 0;
  }
  /* move the index of the first valid byte for the next read */
  SU->su_InputFirst = index;
  ioreq->io_Actual = count;
  
  SU->su_Status &= ~STATUS_BUFFEROVERFLOW;

  if (count == ioreq->io_Length)
    return TRUE;
  
  /* The request could not be satisfied completely */ 
  return FALSE;
}

/**************************************************************************
  Copy data from the buffer where it is collected to the destination buffer
  in the IORequest structure
  The return value indicates whether the request could be satisfied 
  completely or not.
**************************************************************************/

BOOL copyInDataUntilZero(struct SerialUnit * SU, struct IOStdReq * ioreq)
{
  UWORD count = 0;
  UWORD index = SU->su_InputFirst;
  BYTE * Buffer = ioreq->io_Data;
  BOOL end = FALSE;
  
  while (SU->su_InputNextPos != index)
  {
    /* copy one byte */
    Buffer[count] = SU->su_InputBuffer[index];

    /* was that the terminating zero? */
    if (0 == Buffer[count])
    {
      end = TRUE;
      break;
    } 

    count++;
    index++;
    /*  
    **  The buffer is organized in a circular fashion with
    **  length SU->su_InBufLength 
     */
    if (index == SU->su_InBufLength)
      index = 0;
  }
  /* move the index of the first valid byte for the next read */
  SU->su_InputFirst = index;

  SU->su_Status &= ~STATUS_BUFFEROVERFLOW;

  /* whatever is in end represents "satisfied request"(TRUE) or
     "unsatisfied request" (FALSE) */

  if (TRUE == end)
  {
    ioreq->io_Actual = count;
    return TRUE;
  }

  /* make io_Actual point to the next index in the buffer */
  ioreq->io_Actual = count+1;
  return FALSE;
}


struct SerialUnit * findUnit(struct serialbase * SerialDevice, 
                             ULONG unitnum)
{
  struct SerialUnit * su;
  ForeachNode(&SerialDevice->UnitList, su)
  {
    if (su->su_UnitNum == unitnum)
      return su;
  }
  return NULL;
}


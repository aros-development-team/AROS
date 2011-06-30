/****************************************************************************
**  File:       pipebuf.c
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.1
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
*/

#include   <libraries/dos.h>
#include   <libraries/dosextens.h>
#include   <libraries/filehandler.h>
#include   <exec/exec.h>

#include   "pipelists.h"
#include   "pipename.h"
#include   "pipebuf.h"
#include   "pipecreate.h"
#include   "pipesched.h"
#include   "pipe-handler.h"



/*---------------------------------------------------------------------------
** pipebuf.c
** ---------
** This module contains functions that manage the circular buffer for pipes.
**
** Visible Functions
** -----------------
**	PIPEBUF  *AllocPipebuf   (len)
**	ULONG    MoveFromPipebuf (pb, dest, amt)
**	ULONG    MoveToPipebuf   (pb, src, amt)
**
** Macros (in pipebuf.h)
** ---------------------
**	PipebufEmpty (pb)
**	PipebufFull  (pb)
**	FreePipebuf  (pb)
**
** Local Functions
** ---------------
**	- none -
*/



/*---------------------------------------------------------------------------
** AllocPipebuf() returns a pointer to a new PIPEBUF structure if there is
** enough free memory to allocate one with the requested ("len") storage.
** The structure is iinitialized as empty.  Notice that the buffer storage
** area is the tail part of the structure.
*/

PIPEBUF  *AllocPipebuf (len)

ULONG  len;

{ PIPEBUF  *pb = NULL;


  if ( (len > 0) && (len <= MAX_PIPELEN) &&
       ((pb= (PIPEBUF *) AllocMem (sizeof (PIPEBUF) - 1 + len, ALLOCMEM_FLAGS)) != NULL) )
    { pb->head= pb->tail= 0;
      pb->full= FALSE;
      pb->len= len;
    }

  return pb;
}



/*---------------------------------------------------------------------------
** Move bytes from the PIPEBUF to the memory pointed to by "dest".  At most
** "amt" bytes are moved.  The actual number moved is returned.
*/

ULONG  MoveFromPipebuf (pb, dest, amt)

PIPEBUF        *pb;
register BYTE  *dest;
ULONG          amt;

{ register BYTE  *src;
  register LONG  ct;
  ULONG          amtleft;


  if ((amt <= 0) || PipebufEmpty (pb))
    return 0L;

  amtleft= amt;
  src=  pb->buf + pb->tail;

  if (pb->tail >= pb->head)     /* then have to wrap around */
    { if ((ct= (pb->len - pb->tail)) > amtleft)
        ct= amtleft;     /* more than needed in end of pipebuf */

      CopyMem (src, dest, ct);
      pb->tail= (pb->tail + ct) % pb->len;
      amtleft -= ct;

      src= pb->buf + pb->tail;
      dest += ct;
    }

  if ( (amtleft > 0) && (ct= (pb->head - pb->tail)) )
    { if (ct > amtleft)
        ct= amtleft;     /* more than needed */

      CopyMem (src, dest, ct);
      pb->tail += ct;     /* no need to mod */
      amtleft  -= ct;
    }

  pb->full= FALSE;     /* has to be: nonzero amt */

  return (amt - amtleft);
}



/*---------------------------------------------------------------------------
** Move bytes to the PIPEBUF from the memory pointed to by "src".  At most
** "amt" bytes are moved.  The actual number moved is returned.
*/

ULONG  MoveToPipebuf (pb, src, amt)

PIPEBUF        *pb;
register BYTE  *src;
ULONG          amt;

{ register BYTE  *dest;
  register LONG  ct;
  ULONG          amtleft;


  if ((amt <= 0) || PipebufFull (pb))
    return 0L;

  amtleft= amt;
  dest= pb->buf + pb->head;

  if (pb->head >= pb->tail)     /* then have to wrap around */
    { if ((ct= (pb->len - pb->head)) > amtleft)
        ct= amtleft;     /* more than will fit in end of pipebuf */

      CopyMem (src, dest, ct);
      pb->head= (pb->head + ct) % pb->len;
      amtleft -= ct;

      src += ct;
      dest= pb->buf + pb->head;
    }

  if ( (amtleft > 0) && (ct= (pb->tail - pb->head)) )
    { if (ct > amtleft)
        ct= amtleft;     /* more than will fit */

      CopyMem (src, dest, ct);
      pb->head += ct;     /* no need to mod */
      amtleft -= ct;
    }

  pb->full= (pb->head == pb->tail);

  return (amt - amtleft);
}

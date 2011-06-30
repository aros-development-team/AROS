/****************************************************************************
**  File:       pipebuf.h
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.1
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
*/



#define   MAX_PIPELEN   (1L << 24)



typedef struct pipebuf
  { ULONG  head,       /* index of first character */
           tail;       /* index of last character */
    BYTE   full;       /* flag - takes care of full/empty ambiguity */
    ULONG  len;        /* length of buffer */
    BYTE   buf[1];     /* buffer proceeds from here */
  }
PIPEBUF;



#define   PipebufEmpty(pb)   (((pb)->head == (pb)->tail) && (! (pb)->full))
#define   PipebufFull(pb)    (((pb)->head == (pb)->tail) && ((pb)->full))
#define   FreePipebuf(pb)    (FreeMem ((pb), sizeof (PIPEBUF) - 1 + (pb)->len))



extern PIPEBUF  *AllocPipebuf   ( /* len */ );
extern ULONG    MoveFromPipebuf ( /* pb, dest, amt */ );
extern ULONG    MoveToPipebuf   ( /* pb, src, amt */ );

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Area support functions
    Lang: english
*/

#include <memory.h>
#include <proto/alib.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <string.h>

#include <aros/debug.h>

#include "graphics_intern.h"

/*
  The algorithm was taken from: 
  Computer Graphics
  A programming approach, 2n edition
  Steven Harrington
  Xerox Corp.
  
  pages 79-91.


  The algorithm that follows the borderlines has to go hand in hand
  with the algorithm that draws the line, such that no parts are
  filled that aren't supposed to be filled.
 */

void LineInTmpRas(struct RastPort  * rp,
                  struct Rectangle * bounds,
                  UWORD              BytesPerRow,
                  UWORD              xleft,
                  UWORD              xright,
                  UWORD              y,
                  struct GfxBase   * GfxBase);


struct BoundLine
{
  UWORD StartIndex;
  UWORD EndIndex;
  UWORD LeftX;
  UWORD RightX;
   WORD DeltaX;
   WORD DeltaY;
   WORD Count;
   BOOL Valid;
   WORD incrE;
   WORD incrNE;
   WORD s1;
   WORD t;
   WORD horiz;
   WORD NLeftX;
   WORD NRightX;
};

UWORD Include (UWORD lastused, 
               UWORD lastindex,
               struct BoundLine * AreaBound, 
               UWORD scan, 
               UWORD * VctTbl)
{
  while (lastused < lastindex &&
         VctTbl[AreaBound[lastused+1].StartIndex+1] == scan)
  {
/*
    kprintf("including new one! ");
    kprintf("(%d,%d)-(%d,%d)\n",VctTbl[AreaBound[lastused+1].StartIndex],
                                VctTbl[AreaBound[lastused+1].StartIndex+1],
                                VctTbl[AreaBound[lastused+1].EndIndex],
                                VctTbl[AreaBound[lastused+1].EndIndex]);
*/
    lastused++;
  }
  return lastused;
}

void FillScan(UWORD StartIndex,
              UWORD EndIndex,
              struct BoundLine * AreaBound,
              UWORD scanline,
              struct RastPort * rp,
              struct Rectangle * bounds,
              UWORD BytesPerRow,
              struct GfxBase * GfxBase)
{
  int i = StartIndex;
  int x1;
  while (i < EndIndex)
  {
    /* simply draw a line */
      while (FALSE == AreaBound[i].Valid)
      {
        i++;
        if (i > EndIndex) return;
      }
      x1=AreaBound[i].RightX+1;

      while (FALSE == AreaBound[i+1].Valid)
      {
        i++;
        if (i > EndIndex) return;
      }

      if (x1 <= AreaBound[i+1].LeftX-1)
      {
        LineInTmpRas(rp,
                     bounds,
                     BytesPerRow,
                     x1,
                     AreaBound[i+1].LeftX-1,
                     scanline, 
                     GfxBase);
      }

    i+=2;
  }
}


void XSort(UWORD StartIndex,
           UWORD EndIndex,
           struct BoundLine * AreaBound)
{
  /* a simple bubble sort */
  struct BoundLine tmpAreaBound;
  int i = StartIndex+1;

  //kprintf("%d,%d\n",StartIndex,EndIndex);

//kprintf("%d  ",AreaBound[StartIndex].LeftX);

  while (i <= EndIndex)
  {
    if (AreaBound[i].LeftX < AreaBound[i-1].LeftX)
    {
      /* The one at index i needs to go more to smaller indices */
      int i2 = i;
      //kprintf("sorting!!\n");
      tmpAreaBound = AreaBound[i];
      while (TRUE)
      {
        AreaBound[i2] = AreaBound[i2-1];
        i2--;
        if (i2 == StartIndex || 
            AreaBound[i2-1].LeftX <= tmpAreaBound.LeftX )
	{ 
          AreaBound[i2] = tmpAreaBound;
          break;
	}
      }
    }
    i++;
    //kprintf("%d  ",AreaBound[i].LeftX);

  }
  //kprintf("\n");
}


UWORD UpdateXValues(UWORD StartIndex, 
                    UWORD EndIndex,
                    UWORD scan, 
                    struct BoundLine * AreaBound,
                    UWORD * VctTbl)
{
  int i = StartIndex;
  BOOL foundvalid = FALSE;

  while (i <= EndIndex)
  {
    /* Test whether this one is still to be considered */
    // CHANGED <= to <
    if ( VctTbl[AreaBound[i].EndIndex+1] <=  scan ||
         AreaBound[i].Valid == FALSE )
    {
/*
if (AreaBound[i].Valid == FALSE)
  kprintf ("already marked as invalid! ");
else
  kprintf("marking %d as anvalid! ",i);
kprintf("(%d,%d)-(%d,%d)\n",VctTbl[AreaBound[i].StartIndex],
                            VctTbl[AreaBound[i].StartIndex+1],
                            VctTbl[AreaBound[i].EndIndex],
                            VctTbl[AreaBound[i].EndIndex+1]);
*/
      AreaBound[i].Valid = FALSE;
      if (FALSE == foundvalid)
        StartIndex += 1; 
    } else {
      /* It is still to be considered!! */
      foundvalid = TRUE;
      /* calculate the new x-coordinates for the new line */
      if (0 == AreaBound[i].DeltaX)
      {
        /* a vertical line !!! easy!! */
        i++;
        continue;
      }

      AreaBound[i].RightX += AreaBound[i].NRightX;
      AreaBound[i].LeftX  += AreaBound[i].NLeftX;
      AreaBound[i].NRightX = 0;
      AreaBound[i].NLeftX  = 0;
      /*
       * If we're moving more in the horizontal
       * than in the vertical, then the line
       * has a pure horizontal component which I
       * must take care of by not painting over it.
       * This means that on a y coordinate the line
       * can go from the LeftX to the RightX.
       */
      if (1 == AreaBound[i].horiz) {
        /*
         * More towards the horizontal than down
         */
        if (AreaBound[i].s1 > 0) {
          AreaBound[i].LeftX  = AreaBound[i].RightX;
        } else {
          AreaBound[i].RightX = AreaBound[i].LeftX;
        }
      }


      while (1) {
        if (AreaBound[i].Count <= 0) {
          AreaBound[i].Count += AreaBound[i].incrE;
          if (1 == AreaBound[i].t) {
            if (AreaBound[i].s1 > 0) {
              /*
               * Towards right
               */
              AreaBound[i].RightX++;
            } else {
              /*
               * Towards left
               */
              AreaBound[i].LeftX--;
            }
          } else {
            /*
             * Going to next Y coordinate 
             */
            break;
          }
        } else {
          AreaBound[i].Count += AreaBound[i].incrNE;
          /*
           * Going to next Y coordinate 
           */
          if (AreaBound[i].s1 > 0) {
            /*
             * Towards right
             */
            AreaBound[i].NRightX = 1;
          } else {
            /*
             * Towards left
             */
            AreaBound[i].NLeftX = -1;
          }
          break;
        }
      } /* while (1) */

      /*
       * If we're going more vertical than horizontal
       * then the left and right are always the same.
       */
      if (0 == AreaBound[i].horiz) {
        if (AreaBound[i].s1 > 0) {
        /*
          AreaBound[i].RightX += AreaBound[i].NRightX;
          AreaBound[i].NRightX = 0;
         */
          AreaBound[i].LeftX  = AreaBound[i].RightX;
        } else {
        /*
          AreaBound[i].LeftX += AreaBound[i].NLeftX;
          AreaBound[i].NLeftX = 0;
         */
          AreaBound[i].RightX = AreaBound[i].LeftX;
        }
      }
    }
    i++;
  }

  return StartIndex;
}

/* functions for filling of the RastPort */
BOOL areafillpolygon(struct RastPort  * rp,
                     struct Rectangle * bounds,
                     UWORD              first_idx,
                     UWORD              last_idx,
                     ULONG              BytesPerRow,
                     struct GfxBase *   GfxBase)
{
  int i, c;
  UWORD StartEdge = 1;
  UWORD EndEdge = 1;
  WORD LastIndex;
  UWORD ymin;
  UWORD LastEdge = last_idx - first_idx + 1; // needed later on. Don't change!!
  struct AreaInfo * areainfo = rp->AreaInfo;
  UWORD * StartVctTbl = &areainfo->VctrTbl[first_idx * 2];
  UWORD scan;
  struct BoundLine tmpAreaBound;
  struct BoundLine * AreaBound = 
        (struct BoundLine *) AllocMem(sizeof(struct BoundLine) * LastEdge, 
                                      MEMF_CLEAR); 
  
  if (NULL == AreaBound)
   return FALSE;

  /* first clear the buffer of the temporary rastport as far as necessary  */
  
  memset(rp->TmpRas->RasPtr, 
         0, 
         BytesPerRow * (bounds->MaxY - bounds->MinY + 1));
  
/*
  kprintf("first: %d, last: %d\n",first_idx,last_idx);
  kprintf("(%d,%d)-(%d,%d)\n",bounds->MinX,bounds->MinY,
                              bounds->MaxX,bounds->MaxY);
  kprintf("width: %d, bytesperrow: %d\n",bounds->MaxX - bounds->MinX + 1,
                                         BytesPerRow);
*/  
  /* I need a list of sorted indices that represent the lines of the 
  ** polygon. Horizontal lines don't go into that list!!!
  ** The lines are sorted by their start-y coordinates. 
  */

  i = -1;
  c = 0;

  /* process all points of the polygon */
  while (c < (LastEdge-1)*2)
  {
    int i2;
    /* is the next one starting point of a horizontal line??? If yes,
       then skip it */
/*
    kprintf("current idx for y: %d, next idx for y: %d\n",c+1,c+3);
*/
    if (StartVctTbl[c+1] == StartVctTbl[c+3])
    { 
//      kprintf("Found horizontal Line!!\n");
      c+=2;
      continue;
    }

    /* which coordinate of this line has the lower y value */
    if (StartVctTbl[c+1] < StartVctTbl[c+3])
    {
      tmpAreaBound.StartIndex = c;
      tmpAreaBound.EndIndex   = c+2;
      ymin = StartVctTbl[c+1];
    }
    else
    {
      tmpAreaBound.StartIndex = c+2;
      tmpAreaBound.EndIndex   = c;
      ymin = StartVctTbl[c+3];
    }

/*
    kprintf("line: (%d,%d)-(%d,%d)  ",StartVctTbl[c],
                                      StartVctTbl[c+1],
                                      StartVctTbl[c+2],
                                      StartVctTbl[c+3]);
    kprintf("miny: %d\n",ymin);
*/
    i2 = 0;
    /* 
    ** search for the place where to put this entry into the sorted 
    ** (increasing start y-coordinates) list 
    */
    if (i > -1)
    {
      while (TRUE)
      {      
/*
kprintf("ymin: %d< %d?\n",ymin,StartVctTbl[AreaBound[i2].StartIndex+1]);
*/
        if (ymin < StartVctTbl[AreaBound[i2].StartIndex+1])
        {
          int i3 = i+1;
          /* found the place! */
          while (i3 > i2) 
	  {
/*
kprintf("moving!\n");
*/
            AreaBound[i3].StartIndex = AreaBound[i3-1].StartIndex;
            AreaBound[i3].EndIndex   = AreaBound[i3-1].EndIndex;
	    i3--;
          }
          AreaBound[i2].StartIndex = tmpAreaBound.StartIndex;
          AreaBound[i2].EndIndex   = tmpAreaBound.EndIndex;
          break;
        }
        i2++;
        if (i2 > i)
	{
/*
kprintf("at end!\n");
*/
          AreaBound[i+1].StartIndex = tmpAreaBound.StartIndex;
          AreaBound[i+1].EndIndex   = tmpAreaBound.EndIndex;
          break;
        }
      }
    }
    else /* first one to insert into list */
    {
      AreaBound[0].StartIndex = tmpAreaBound.StartIndex;
      AreaBound[0].EndIndex   = tmpAreaBound.EndIndex;
    }
    c += 2;
    i++;
  }

  LastIndex = i;
  i = 0;

/*
  {
    int i2 = 0;
    while (i2 <= LastIndex)
    {
      kprintf("%d.: index %d (%d,%d)-(%d,%d)\n",i2,AreaBound[i2].StartIndex, 
                               StartVctTbl[AreaBound[i2].StartIndex], 
                               StartVctTbl[AreaBound[i2].StartIndex+1],
                               StartVctTbl[AreaBound[i2].EndIndex],
                               StartVctTbl[AreaBound[i2].EndIndex+1]);
      i2++;
    }
  }
*/

  while (i <= LastIndex)
  {
    int StartIndex = AreaBound[i].StartIndex;
    int EndIndex   = AreaBound[i].EndIndex;

    if ((StartVctTbl[EndIndex] - StartVctTbl[StartIndex]) > 0) {
      AreaBound[i].s1 = 1;
    } else {
      AreaBound[i].s1 = -1;
    }

    AreaBound[i].DeltaX = abs(StartVctTbl[EndIndex] -
                              StartVctTbl[StartIndex]);

    AreaBound[i].DeltaY = abs(StartVctTbl[EndIndex+1] - 
                              StartVctTbl[StartIndex+1]);

    if (AreaBound[i].DeltaX > AreaBound[i].DeltaY) {
      AreaBound[i].horiz = 1;
    }


    if (AreaBound[i].DeltaX < AreaBound[i].DeltaY) {
      WORD d = AreaBound[i].DeltaX;
      AreaBound[i].DeltaX = AreaBound[i].DeltaY;
      AreaBound[i].DeltaY = d;
      AreaBound[i].t = 0;
    } else {
      AreaBound[i].t = 1;
    }

    AreaBound[i].Count =  (AreaBound[i].DeltaY * 2) - AreaBound[i].DeltaX;
    AreaBound[i].incrE  =  AreaBound[i].DeltaY * 2;
    AreaBound[i].incrNE = (AreaBound[i].DeltaY - AreaBound[i].DeltaX) * 2;

    AreaBound[i].LeftX = AreaBound[i].RightX = StartVctTbl[StartIndex];
    AreaBound[i].Valid = TRUE;
    i++;
  }
  
  /* indexlist now contains i+1 indices into the vector table.
     Either the coordinate at the index as declared in the indexlist
     contains the lower y value or the following coordinate */

  scan = bounds->MinY;

  LastIndex = i;

  StartEdge = 0;
  EndEdge = Include(1, LastIndex, AreaBound, scan, StartVctTbl);
  StartEdge = UpdateXValues(StartEdge, EndEdge, scan, AreaBound, StartVctTbl);

  while (scan < bounds->MaxY)
  {
    XSort(StartEdge, EndEdge, AreaBound);

    if (scan > bounds->MinY)
      FillScan(StartEdge, 
               EndEdge, 
               AreaBound, 
               scan, 
               rp, 
               bounds, 
               BytesPerRow,
               GfxBase);

/*
    kprintf("scanline: %d   StartEdge: %d, EndEdge: %d\n",scan,StartEdge,EndEdge);

    {
      int x = StartEdge;
      while (x <= EndEdge)
      { 
        if (TRUE == AreaBound[x].Valid)
        {
          kprintf("(%d,%d)-(%d,%d) currently at: Left: %d Right: %d\n",
                      StartVctTbl[AreaBound[x].StartIndex],
                      StartVctTbl[AreaBound[x].StartIndex+1],
                      StartVctTbl[AreaBound[x].EndIndex],
                      StartVctTbl[AreaBound[x].EndIndex+1],
                      AreaBound[x].LeftX,
                      AreaBound[x].RightX);
	}
        else
          kprintf("invalid\n");
        x++;
      }
    }
*/
    scan++;
    EndEdge = Include(EndEdge, LastIndex, AreaBound, scan, StartVctTbl);
    StartEdge = UpdateXValues(StartEdge, EndEdge, scan, AreaBound, StartVctTbl);
/*
    kprintf("StartEdge: %d, EndEdge: %d\n",StartEdge,EndEdge);
*/
  }

  FreeMem( AreaBound, sizeof(struct BoundLine) * LastEdge); 

  return TRUE;
}


void areafillellipse(struct RastPort  * rp,
                     struct Rectangle * bounds,
                     UWORD            * CurVctr,
                     ULONG              BytesPerRow,
                     struct GfxBase *   GfxBase)
{
  /* the ellipse drawing algorithm is taken from DrawEllipse() */
  LONG x = CurVctr[2], y = 0;   /* ellipse points */
  
  /* intermediate terms to speed up loop */
  LONG t1 = CurVctr[2] * CurVctr[2], t2 = t1 << 1, t3 = t2 << 1;
  LONG t4 = CurVctr[3] * CurVctr[3], t5 = t4 << 1, t6 = t5 << 1;
  LONG t7 = CurVctr[2] * t5, t8 = t7 << 1, t9 = 0L;
  LONG d1 = t2 - t7 + (t4 >> 1);  /* error terms */
  LONG d2 = (t1 >> 1) - t8 + t5;

  memset(rp->TmpRas->RasPtr,
         0x00,
         BytesPerRow * (bounds->MaxY - bounds->MinY + 1) );
/*
kprintf("filled bytes: %d\n",BytesPerRow * (bounds->MaxY - bounds->MinY + 1));

kprintf("Filling ellipse with center at (%d,%d) and radius in x: %d and in y: %d\n",
                            CurVctr[0],CurVctr[1],CurVctr[2],CurVctr[3]);
*/  
  while (d2 < 0 && y < CurVctr[3])
  {
    /* draw 2 lines using symmetry */
    if (x > 1)
    {
      LineInTmpRas(rp,
                   bounds,
                   BytesPerRow,
                   CurVctr[0] - x + 1,
                   CurVctr[0] + x - 1,
                   CurVctr[1] - y,
                   GfxBase);

      LineInTmpRas(rp,
                   bounds,
                   BytesPerRow,
                   CurVctr[0] - x + 1,
                   CurVctr[0] + x - 1,
                   CurVctr[1] + y,
                   GfxBase);
    } 
    
    y++;            /* always move up here */
    t9 = t9 + t3;
    if (d1 < 0)     /* move straight up */
    {
      d1 = d1 + t9 + t2;
      d2 = d2 + t9;
    }
    else
    {
      x--;
      t8 = t8 - t6;
      d1 = d1 + t9 + t2 - t8;
      d2 = d2 + t9 + t5 - t8;
    }
  }
  
  do                /* rest of the right quadrant */
  {
    /* draw 2 lines using symmetry */
   
    x--;         /* always move left here */
    t8 = t8 - t6;
    if (d2 < 0)  /* move up and left */
    { 
      if (x > 1)
      {
      LineInTmpRas(rp,
                   bounds,
                   BytesPerRow,
                   CurVctr[0] - x + 1,
                   CurVctr[0] + x - 1,
                   CurVctr[1] - y,
                   GfxBase);

      LineInTmpRas(rp,
                   bounds,
                   BytesPerRow,
                   CurVctr[0] - x + 1,
                   CurVctr[0] + x - 1,
                   CurVctr[1] + y,
                   GfxBase);
      }
      else 
        break; 

      y ++;
      t9 = t9 + t3;
      d2 = d2 + t9 + t5 - t8;
    } 
    else        /* move straight left */
    {
      d2 = d2 + t5 - t8;
    } 
  } while ( x > 0 && y < CurVctr[3] );
}

/*
** Draw a horizontal line into a temporary rastport.
*/

void LineInTmpRas(struct RastPort  * rp,
                  struct Rectangle * bounds,
                  UWORD              BytesPerRow,
                  UWORD              xleft,
                  UWORD              xright,
                  UWORD              y,
                  struct GfxBase   * GfxBase)
{
  UWORD  index;
  UWORD  NumPixels;
  WORD   PixelMask;
  UWORD  PixelMask2;
  UWORD * RasPtr = (WORD *)rp->TmpRas->RasPtr;
  ULONG  shift;

//kprintf("(%d/%d) to (%d/%d)\n",xleft,y,xright,y);

  /* adjust the coordinates */
  xleft  -= bounds->MinX;
  xright -= bounds->MinX; 
  y      -= bounds->MinY;

  //kprintf("line from %d to %d y = %d\n",xleft,xright,y);

  if (xleft > xright) return;
  /* 
    an algorithm that tries to minimize the number of accesses to the 
    RasPtr
  */
  
  /* Fill the first word */
  PixelMask = 0x8000;
  
  /* determine the number of pixels to set at the beginning */
  NumPixels = xright - xleft + 1;
  if (NumPixels > 16)
    NumPixels = 16;
  
  /* create enough pixels */  
  PixelMask >>= (NumPixels - 1);

  index = (y * (BytesPerRow >> 1)) + (xleft >> 4);
  /* Adjust the pixelmask so we hit the very first pixel  */
  PixelMask2 = PixelMask & 0xffff;
  if (0 != (shift = (xleft & 0x0f))) {
    PixelMask2 >>= shift;
  }
  
#if (AROS_BIG_ENDIAN == 0)
  /* Endianess conversion*/
  PixelMask2 = PixelMask2 << 8 | PixelMask2 >> 8;    
#endif
  RasPtr[index] |= PixelMask2;
//kprintf("%x (left)\n",PixelMask2);
  
  index++;
  
  xleft = xleft + (16 - shift);
 
  if ((xright - xleft) < 16)
    goto fillright;

  /* fill the middle with 0xffff's */
  while ((xleft + 15) < xright)
  {
    RasPtr[index] = (WORD)0xffff;
    index++;
    xleft += 16;
  }

fillright:  
  if (xleft <= xright)
  {
    PixelMask = 0x8000;
    /* Create enough pixels - one pixel is already there! */
    if (0 != (shift = (xright - xleft + 0))) {
      PixelMask >>= shift;
    }

    PixelMask2 = PixelMask & 0xffff;

#if (AROS_BIG_ENDIAN == 0)
    /* Endianess conversion*/
    PixelMask2 = PixelMask2 << 8 | PixelMask2 >> 8;
#endif    

    RasPtr[index] |= PixelMask2;
//kprintf("%x (right)\n",PixelMask2);
  }

}


void areaclosepolygon(struct AreaInfo *areainfo)
{
    /* Note: the caller must make sure, that this
       function is only called if areainfo->Count > 0
       and that there is place for one vector
       (areainfo->Count < areainfo->MaxCount) */
       
    if ( areainfo->FlagPtr[-1] == AREAINFOFLAG_DRAW )
    {
	if ((areainfo->VctrPtr[-1] != areainfo->FirstY) ||
            (areainfo->VctrPtr[-2] != areainfo->FirstX))
	{
	    areainfo->Count++;
	    areainfo->VctrPtr[0] = areainfo->FirstX;
	    areainfo->VctrPtr[1] = areainfo->FirstY;
    	    areainfo->FlagPtr[0] = AREAINFOFLAG_CLOSEDRAW;

	    areainfo->VctrPtr = &areainfo->VctrPtr[2];
	    areainfo->FlagPtr++;
	}
	else
	{
	    areainfo->FlagPtr[-1] = AREAINFOFLAG_CLOSEDRAW;
	}
    }
}


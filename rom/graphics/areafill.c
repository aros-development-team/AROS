#include <memory.h>
#include <proto/alib.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>

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
};

UWORD Include (UWORD lastused, 
               UWORD lastindex,
               struct BoundLine * AreaBound, 
               UWORD scan, 
               UWORD * VctTbl)
{
  while (VctTbl[AreaBound[lastused+1].StartIndex+1] == scan)
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
      Move(rp, x1,  scanline);

      SetAPen(rp,8);

      while (FALSE == AreaBound[i+1].Valid)
      {
        i++;
        if (i > EndIndex) return;
      }

      if (x1 <= AreaBound[i+1].LeftX-1)
        Draw(rp, AreaBound[i+1].LeftX-1, scanline);
/*
      else
        kprintf("Refusing to draw from %d to %d\n",x1,AreaBound[i+1].LeftX-1);
*/
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
    if ( VctTbl[AreaBound[i].EndIndex+1] <= scan ||
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
    }
    else
    {
      /* It is still to be considered!! */
      foundvalid = TRUE;
      /* calculate the new x-coordinates for the new line */
      if (0 == AreaBound[i].DeltaX)
      {
        /* a vertical line !!! easy!! */
        i++;
        continue;
      }
     
      /* line towards right? */
      if  (AreaBound[i].DeltaX > 0)
      {
        if (AreaBound[i].DeltaX > AreaBound[i].DeltaY)
        {
          /* more towards right than down */
          AreaBound[i].RightX++;
          AreaBound[i].LeftX = AreaBound[i].RightX;

          while (TRUE)
          {
            /* Now search for the right X coord. */
            AreaBound[i].Count += AreaBound[i].DeltaY;

            if (AreaBound[i].Count > AreaBound[i].DeltaX)
	    {
              AreaBound[i].Count -= AreaBound[i].DeltaX;
              break;
 	    }

            /* we're going towards the right in (almost) every step. */
            AreaBound[i].RightX++;

          }        
	}
        else
        {
          /* in every calculation we go down one scan line */
          /* LeftX == RightX at all times!! */
          AreaBound[i].Count += AreaBound[i].DeltaX;
          
          if (AreaBound[i].Count > AreaBound[i].DeltaY)
	  {
            AreaBound[i].Count -= AreaBound[i].DeltaY;
            AreaBound[i].LeftX++;
            AreaBound[i].RightX++;
	  }
	}
      }
      else /* line goes towards left */
      {
        if (-AreaBound[i].DeltaX > AreaBound[i].DeltaY)
        {
          /* more towards left than down */
          AreaBound[i].LeftX--;
          AreaBound[i].RightX = AreaBound[i].LeftX;

          while (TRUE)
          {
            /* Now search for the left X coord. */
            AreaBound[i].Count += AreaBound[i].DeltaY;

            if (AreaBound[i].Count >= -AreaBound[i].DeltaX)
	    {
              AreaBound[i].Count += AreaBound[i].DeltaX;
              break;
 	    }

            /* we're going towards the left in (almost) every step. */
            AreaBound[i].LeftX--;
          }        
	}
        else
        {
          /* in every calculation we go down one scan line */
          /* LeftX == RightX at all times!! */
          AreaBound[i].Count -= AreaBound[i].DeltaX;
          
          if (AreaBound[i].Count >= AreaBound[i].DeltaY)
	  {
            AreaBound[i].Count -= AreaBound[i].DeltaY;
            AreaBound[i].LeftX--;
            AreaBound[i].RightX--;
	  }
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
                     UWORD              BytesPerRow,
                     struct GfxBase *   GfxBase)
{
  int i, c;
  UWORD StartEdge = 1;
  UWORD EndEdge = 1;
  UWORD LastIndex;
  UWORD ymin;
  UWORD LastEdge = last_idx - first_idx + 1; // needed later on. Don't change!!
  struct AreaInfo * areainfo = rp->AreaInfo;
  UWORD * StartVctTbl = areainfo->VctrTbl;
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
//      kprintf("Found horiontal Line!!\n");
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
    ** (incresing start y-coordinates) list 
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

    AreaBound[i].DeltaX = StartVctTbl[EndIndex] - 
                          StartVctTbl[StartIndex];

    AreaBound[i].LeftX = AreaBound[i].RightX = StartVctTbl[StartIndex];

    AreaBound[i].DeltaY = StartVctTbl[EndIndex+1] - 
                          StartVctTbl[StartIndex+1]   + 1;

    if (0 != AreaBound[i].DeltaX )
    {
      if (AreaBound[i].DeltaX > 0)
      {
        AreaBound[i].DeltaX++;

        if (AreaBound[i].DeltaX > AreaBound[i].DeltaY)
	{
          while (TRUE)
	  {
            /* search for the right-hand X coord. */
            AreaBound[i].Count += AreaBound[i].DeltaY;

            if (AreaBound[i].Count > AreaBound[i].DeltaX)
	    {
              AreaBound[i].Count -= AreaBound[i].DeltaX;
              break;
	    }

	    /* we're going towards the right in (almost) every step */
            AreaBound[i].RightX++;
	  }
	}
        else
          AreaBound[i].Count = AreaBound[i].DeltaX;
      }
      else
      {
        AreaBound[i].DeltaX--;
        if (-AreaBound[i].DeltaX > AreaBound[i].DeltaY)
	{
          AreaBound[i].Count = AreaBound[i].DeltaY;
          while (TRUE)
	  {
            /* serach for the left X coord */
            AreaBound[i].Count += AreaBound[i].DeltaY;
            if (AreaBound[i].Count > -AreaBound[i].DeltaX)
	    {
              AreaBound[i].Count += AreaBound[i].DeltaX;
              break;
	    }

            /* we're going towards the left in (almost) every step */
            AreaBound[i].LeftX--;

	  }
	}
        else
          AreaBound[i].Count = -AreaBound[i].DeltaX;
      }
    }    

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

  while (scan < bounds->MaxY)
  {
    XSort(StartEdge, EndEdge, AreaBound);

    if (scan > bounds->MinY)
      FillScan(StartEdge, EndEdge, AreaBound, scan, rp, GfxBase);

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
    StartEdge = UpdateXValues(StartEdge, EndEdge, scan, AreaBound, StartVctTbl);
    EndEdge = Include(EndEdge, LastIndex, AreaBound, scan, StartVctTbl);
/*
    kprintf("StartEdge: %d, EndEdge: %d\n",StartEdge,EndEdge);
*/
  }

  FreeMem( AreaBound, sizeof(struct BoundLine) * LastEdge); 

  return TRUE;
}

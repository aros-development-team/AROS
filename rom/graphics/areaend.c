/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AreaEnd()
    Lang: english
*/
#include <exec/types.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(LONG, AreaEnd,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 44, Graphics)

/*  FUNCTION
        Process the filled vector buffer. 
        After the operation the buffer is reinitilized for
        processing of further Area functions.
        Makes use of the raster given by the TmpRas structure that
        is linked to the rastport.

    INPUTS
	rp - pointer to a valid RastPort structure with a pointer to
	     the previously initilized AreaInfo structure.


    RESULT
	error -  0 for success
	        -1 a error occurred

    NOTES

    EXAMPLE

    BUGS
        There is still a problem when some polygons are filled that
        pixels are missing. This could be due to the way lines are 
        drawn. All lines should be drawn from lower
        y coordinates to higher y coordinates since this is the
        way the alogrithm calculates lines here. For example, it
        might make a difference whether a line is drawn from lower
        to higher y coordinates. Examples for two same lines with
        different layout:
        
             ****              *****
        *****              ****
        

    SEE ALSO
	InitArea() AreaDraw() AreaEllipse() AreaCircle() graphics/rastport.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct AreaInfo * areainfo = rp->AreaInfo;

    /* is there anything in the matrix at all ? And do we have a TmpRas in the rastport? */
    if (areainfo->Count && rp->TmpRas)
    {
	WORD first_idx = 0;
	WORD last_idx  = -1;
	ULONG BytesPerRow;
	UWORD * CurVctr  = areainfo -> VctrTbl;
	BYTE  * CurFlag  = areainfo -> FlagTbl;
	UWORD Count;
	UWORD Rem_APen   = GetAPen(rp);
	/* I don't know whether this function may corrupt the 
	   cursor position of the rastport. So I save it for later.*/

	UWORD Rem_cp_x   = rp->cp_x;
	UWORD Rem_cp_y   = rp->cp_y;
	/* This rectangle serves as a "frame" for the tmpras for filling */
	struct Rectangle bounds;


	areaclosepolygon(areainfo);

	Count = areainfo->Count;  

    	//kprintf("%d coord to process\n",Count);
    
	/* process the list of vectors */
	while (Count > 0)
	{
    	    //kprintf("\n******** Flags:%d Coord: (%d,%d)\n",CurFlag[0], CurVctr[0],CurVctr[1]);

	    last_idx ++;
	    switch((unsigned char)CurFlag[0])
	    {
        	case AREAINFOFLAG_MOVE:
		    /* set the graphical cursor to a starting position */
        	    Move(rp, CurVctr[0], CurVctr[1]);

        	    bounds.MinX = CurVctr[0];
        	    bounds.MaxX = CurVctr[0];
        	    bounds.MinY = CurVctr[1];
        	    bounds.MaxY = CurVctr[1];

        	    CurVctr = &CurVctr[2];
        	    CurFlag = &CurFlag[1];
        	    break;

        	case AREAINFOFLAG_CLOSEDRAW:
        	    /* this indicates that this Polygon is closed with this coordinate */
        	    /*
        	     * Must draw from lower y's to higher ones otherwise
        	     * the fill algo does not work nicely.
        	     */
#if 1
        	    if (rp->cp_y <= CurVctr[1]) {
        	      Draw(rp, CurVctr[0], CurVctr[1]);
        	    } else {
        	      int _x = rp->cp_x;
        	      int _y = rp->cp_y;
        	      rp->cp_x = CurVctr[0];
        	      rp->cp_y = CurVctr[1];
        	      Draw(rp, _x, _y);
        	      rp->cp_x = CurVctr[0];
        	      rp->cp_y = CurVctr[1];
        	    }
#endif
        	    CurVctr = &CurVctr[2];
        	    CurFlag = &CurFlag[1];
        	    /* 
        	       no need to set the bundaries here like in case above as
        	       this coord closes the polygon and therefore is the same
        	       one as the first coordinate of the polygon. 
        	    */
        	    /* check whether there's anything to fill at all. I cannot
        	       fill a line (=3 coordinates) */
        	    if (first_idx+2 <= last_idx)
		    {
        		/* BytesPerRow must be a multiple of 2 bytes */

        		BytesPerRow = bounds.MaxX - bounds.MinX + 1;
        		if (0 != (BytesPerRow & 0x0f ))
                	    BytesPerRow =((BytesPerRow >> 3) & 0xfffe )+ 2;
        		else
                	    BytesPerRow = (BytesPerRow >> 3) & 0xfffe;

    	    	    	if ((ULONG)rp->TmpRas->Size < BytesPerRow * (bounds.MaxY - bounds.MinY + 1))
		    	    return -1;

	  /*              
        		kprintf("first: %d, last: %d\n",first_idx,last_idx);
        		kprintf("(%d,%d)-(%d,%d)\n",bounds.MinX,bounds.MinY,
                                        	    bounds.MaxX,bounds.MaxY);
        		kprintf("width: %d, bytesperrow: %d\n",bounds.MaxX-bounds.MinX+1,
                                                	       BytesPerRow);
	  */
#if 1
        		if (TRUE == areafillpolygon(rp,
                                        	    &bounds, 
                                        	    first_idx, 
                                        	    last_idx,
                                        	    BytesPerRow,
                                        	    GfxBase))
			{
                	    /* 
                	     Blit the area fill pattern through the mask provided
                	     by rp->TmpRas.
                	    */

                	    BltPattern(
                	       rp,
                	       rp->TmpRas->RasPtr,
                	       bounds.MinX,
                	       bounds.MinY,
                	       bounds.MaxX,
                	       bounds.MaxY,
                	       BytesPerRow
                	    );

			    if  (rp->Flags & AREAOUTLINE)
			    {
				SetAPen(rp, GetOutlinePen(rp));
				PolyDraw(rp, last_idx - first_idx + 1, &areainfo->VctrTbl[first_idx]);
				SetAPen(rp, Rem_APen);		    
			    }

			}
#endif
		    }
        	    /* set first_idx for a possible next polygon to draw */
        	    first_idx = last_idx + 1;
        	    break;

        	case AREAINFOFLAG_DRAW:
        	    /* Draw a line to new position */
#if 1
        	    /*
        	     * Must draw from lower y's to higher ones otherwise
        	     * the fill algo does not work nicely.
        	     */
        	    if (rp->cp_y <= CurVctr[1]) {
        	      Draw(rp, CurVctr[0], CurVctr[1]);
        	    } else {
        	      int _x = rp->cp_x;
        	      int _y = rp->cp_y;
        	      rp->cp_x = CurVctr[0];
        	      rp->cp_y = CurVctr[1];
        	      Draw(rp, _x, _y);
        	      rp->cp_x = CurVctr[0];
        	      rp->cp_y = CurVctr[1];
        	    }
#endif
        	    if (bounds.MinX > CurVctr[0])
        	        bounds.MinX = CurVctr[0];
        	    if (bounds.MaxX < CurVctr[0])
        	        bounds.MaxX = CurVctr[0];
        	    if (bounds.MinY > CurVctr[1])
        	        bounds.MinY = CurVctr[1];
        	    if (bounds.MaxY < CurVctr[1])
        	        bounds.MaxY = CurVctr[1];
        	    CurVctr = &CurVctr[2];
        	    CurFlag = &CurFlag[1];
        	    break;

        	case AREAINFOFLAG_ELLIPSE:
        	    bounds.MinX = CurVctr[0] - CurVctr[2];
        	    bounds.MaxX = CurVctr[0] + CurVctr[2];
        	    bounds.MinY = CurVctr[1] - CurVctr[3];
        	    bounds.MaxY = CurVctr[1] + CurVctr[3];
        	    BytesPerRow = bounds.MaxX - bounds.MinX + 1;

        	    if (0 != (BytesPerRow & 0x0f ))
        	        BytesPerRow =((BytesPerRow >> 3) & 0xfffe )+ 2;
        	    else
        	        BytesPerRow = (BytesPerRow >> 3) & 0xfffe;

    	    	    if ((ULONG)rp->TmpRas->Size < BytesPerRow * (bounds.MaxY - bounds.MinY + 1))
		    	return -1;
			
        	    /* Draw an Ellipse and fill it */
        	    /* see how the data are stored by the second entry */
        	    /* I get cx,cy,cx+a,cy+b*/

        	    DrawEllipse(rp,CurVctr[0], 
                        	   CurVctr[1],
                        	   CurVctr[2],
                        	   CurVctr[3]);

        	    /* area-fill the ellipse with the pattern given
        	       in rp->AreaPtrn , AreaPtSz */

        	    areafillellipse(rp,
	    			    &bounds,
                        	    CurVctr,
                        	    BytesPerRow,
                        	    GfxBase);
        	      /* 
                	Blit the area fill pattern through the mask provided
                	by rp->TmpRas.
        	      */

        	    BltPattern(
                	   rp,
                	   rp->TmpRas->RasPtr,
                	   bounds.MinX,
                	   bounds.MinY,
                	   bounds.MaxX,
                	   bounds.MaxY,
                	   BytesPerRow
                	);

    		    if (rp->Flags & AREAOUTLINE)
		    {
	    		SetAPen(rp, GetOutlinePen(rp));

        		DrawEllipse(rp,CurVctr[0], 
                        	       CurVctr[1],
                        	       CurVctr[2],
                        	       CurVctr[3]);

    	    		SetAPen(rp, Rem_APen);
		    }

        	    CurVctr = &CurVctr[4];
        	    CurFlag = &CurFlag[2];
        	    Count--;
        	    last_idx++; /* there were two coords here! */
        	    
		    /* set first_idx for a possible next polygon to draw */
        	    first_idx = last_idx + 1;
        	    break;

		default:
        	    /* this is an error */
        	    SetAPen(rp, Rem_APen);
        	    /* also restore old graphics cursor position */
        	    rp->cp_x = Rem_cp_x;
        	    rp->cp_y = Rem_cp_y;
        	    return -1;
		    
	    } /* switch((unsigned char)CurFlag[0]) */
	    Count--;

	} /* while (Count > 0) */
	
	/* restore areainfo structure for a new beginning */
	areainfo->VctrPtr = areainfo->VctrTbl;
	areainfo->FlagPtr = areainfo->FlagTbl;
	areainfo->Count   = 0;

	/* restore old APen */
	SetAPen(rp, Rem_APen);    
	/* also restore old graphics cursor position */
	rp->cp_x = Rem_cp_x;
	rp->cp_y = Rem_cp_y;
      
    } /* if vectorlist is not empty and rastport has a tmpras */

    return 0;

    AROS_LIBFUNC_EXIT
    
} /* AreaEnd */

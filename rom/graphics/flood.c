/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>

#include <proto/graphics.h>
#include <string.h>
#include "graphics_intern.h"

#define USE_WRITEPIXEL

struct fillinfo
{
    ULONG fillpen;
    BOOL (*isfillable)();
    struct RastPort *rp;
    UBYTE *rasptr;
    ULONG bpr;
    ULONG orig_apen;
    ULONG orig_bpen;
    ULONG rp_width;
    ULONG rp_height;
    
    struct GfxBase *gfxbase;
};

static VOID settmpraspixel(BYTE *rasptr, LONG x, LONG y,  ULONG bpr, UBYTE state);
static BOOL gettmpraspixel(BYTE *rasptr, LONG x, LONG y,  ULONG bpr );

static BOOL filline(struct fillinfo *fi, LONG start_x, LONG start_y);
static BOOL outline_isfillable(struct fillinfo *fi, LONG x, LONG y);
static BOOL color_isfillable(struct fillinfo *fi, LONG x, LONG y);

#if DEBUG_FLOOD
static int fail_count;
static int pix_written;
#endif

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH4(BOOL, Flood,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(ULONG            , mode, D2),
	AROS_LHA(LONG             , x, D0),
	AROS_LHA(LONG             , y, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 55, Graphics)

/*  FUNCTION
	Flood fill a RastPort.

    INPUTS
	rp   - destination RastPort
	mode - 0: fill adjacent pixels which don't have color of OPen.
	       1: fill adjacent pixels which have the same pen as of coordinate x,y.
	x,y  - coordinate to start filling.

    RESULT

    NOTES
	The RastPort must have a TmpRas raster whose size is as large as of
	that of the RastPort.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct TmpRas *tmpras = rp->TmpRas;
    ULONG bpr, needed_size;
    
    struct fillinfo fi;
    
    BOOL success;
    
    EnterFunc(bug("Flood(rp=%p, mode=%d, x=%d, y=%d)\n"
    		, rp, mode, x, y));

#if DEBUG_FLOOD
    fail_count = 0;
    pix_written = 0;
#endif
    
    /* Check for tmpras */
    if (NULL == tmpras)
    	ReturnBool("Flood (No tmpras)",  FALSE);

    if (NULL != rp->Layer)
    {
    	fi.rp_width  = rp->Layer->Width;
    	fi.rp_height = rp->Layer->Height;
    }
    else
    {
    	fi.rp_width  = GetBitMapAttr(rp->BitMap, BMA_WIDTH);
    	fi.rp_height = GetBitMapAttr(rp->BitMap, BMA_HEIGHT);
    }
    
    
    bpr = WIDTH_TO_BYTES( fi.rp_width );
    needed_size = bpr * fi.rp_height;
    
    if (tmpras->Size < needed_size)
    	ReturnBool("Flood (To small tmpras)",  FALSE);
    

    /* Clear the needed part of tmpras */

/*    
    
!!! Maybe we should use BltClear() here, since 
!!! tmpras allways reside in CHIP RAM
     */
     
    D(bug("Clearing tmpras\n"));
    memset(tmpras->RasPtr, 0,  needed_size);
    
    if (mode == 0)
    {
    	/* Outline mode */
	D(bug("Getting outline pen\n"));
	fi.fillpen = GetOutlinePen(rp);
	D(bug("Got pen\n"));
	
	fi.isfillable = outline_isfillable;
    }
    else
    {
    	/* Color mode */
	D(bug("Reading pixel\n"));
	fi.fillpen = ReadPixel(rp, x, y);
	D(bug("pixel read: %d\n", fi.fillpen));
	fi.isfillable = color_isfillable;
    }
    
    fi.rasptr = tmpras->RasPtr;
    fi.rp = rp;
    fi.bpr = bpr;
    
    fi.orig_apen = GetAPen(rp);
    fi.orig_bpen = GetBPen(rp);
    
    fi.gfxbase = GfxBase;
    
    D(bug("Calling filline\n"));
    success = filline(&fi, x, y);
   
#if DEBUG_FLOOD
    D(bug("fails: %d, pix written: %d\n", fail_count, pix_written));
#endif
    
    SetAPen(rp, fi.orig_apen);
    
    ReturnBool("Flood",  success);
	
    AROS_LIBFUNC_EXIT
} /* Flood */




static VOID settmpraspixel(BYTE *rasptr, LONG x, LONG y,  ULONG bpr, UBYTE state)
{
    ULONG idx  = COORD_TO_BYTEIDX(x, y, bpr);
    UBYTE mask = XCOORD_TO_MASK( x );
    
    if (state)
	rasptr[idx] |= mask;
    else
    	rasptr[idx] &= ~mask;
    
    return;
}

static BOOL gettmpraspixel(BYTE *rasptr, LONG x, LONG y,  ULONG bpr )
{
    ULONG idx  = COORD_TO_BYTEIDX(x, y, bpr);
    UBYTE mask = XCOORD_TO_MASK( x );
    BOOL state;
    
/* D(bug("gettmpraspixel (%d, %d, %d): idx=%d, mask=%d, rasptr[idx]=%d, state=%d\n"
		,x, y, bpr, idx, mask, rasptr[idx], rasptr[idx] & mask));
*/
    state = ((rasptr[idx] & mask) != 0);

/* D(bug("Returning %d\n", state));    
*/
    return state;
}


#undef GfxBase
#define GfxBase (fi->gfxbase)

static BOOL color_isfillable(struct fillinfo *fi, LONG x, LONG y)
{
    BOOL fill;

    if (x < 0 || y < 0 || x >= fi->rp_width || y >= fi->rp_height)
        return FALSE;

    if (gettmpraspixel(fi->rasptr, x, y, fi->bpr))
    {
/*    	D(bug("Pixel checked twice at (%d, %d)\n", x, y)); */
	fill = FALSE;
#if DEBUG_FLOOD
	fail_count ++;
#endif
    }
    else
    {
	fill = (fi->fillpen == ReadPixel(fi->rp, x, y));
    }
	
    return fill;
}

static BOOL outline_isfillable(struct fillinfo *fi, LONG x, LONG y)
{
    BOOL fill;
/*    EnterFunc(bug("outline_isfillable(fi=%p, x=%d, y=%d)\n",
    	fi, x, y));
*/    
    if (x < 0 || y < 0 || x >= fi->rp_width || y >= fi->rp_height)
        return FALSE;

    if (gettmpraspixel(fi->rasptr, x, y, fi->bpr))
    {
/*    	D(bug("Pixel checked twice at (%d, %d)\n", x, y)); */
	fill = FALSE;
#if DEBUG_FLOOD
	fail_count ++;
#endif
    }
    else
    {
	fill = (fi->fillpen != ReadPixel(fi->rp, x, y));
    }
	
/*    D(bug("fillpen: %d, pen: %d\n", fi->fillpen, ReadPixel(fi->rp, x, y)));
*/	
	
/*    ReturnBool("outline_isfillable", fill);
*/
    return fill;
}



static VOID putfillpixel(struct fillinfo *fi, LONG x, LONG y)
{

    /* TODO: Implement use of patterns */

#ifdef USE_WRITEPIXEL
    ULONG pixval, set_pixel = 0UL;
    
    if (fi->rp->AreaPtrn)
    {
    	set_pixel = pattern_pen(fi->rp
		, x, y
		, fi->orig_apen
		, fi->orig_bpen
		, &pixval
		, GfxBase);
    }
    else
    {
        pixval = GetAPen(fi->rp);
        set_pixel = TRUE;
    }
    
    if (set_pixel)
    {
        SetAPen(fi->rp, pixval);
        WritePixel(fi->rp, x, y);
    }
    
#endif

    settmpraspixel(fi->rasptr, x, y, fi->bpr, 1);

#if DEBUG_FLOOD
    pix_written ++;
#endif
    return;
}

#define STACKSIZE 100

struct stack
{
    ULONG current;
    struct scanline
    {
       LONG x, y;
    } items [STACKSIZE];
};

static VOID init_stack(struct stack *s)
{
    s->current = 0;
}

static BOOL push(struct stack *s, LONG x, LONG y)
{
   if (s->current == STACKSIZE)
   	return FALSE;
	
   s->items[s->current].x = x;
   s->items[s->current].y = y;


   s->current ++;
   
   return TRUE;
   
}
static BOOL pop(struct stack *s, LONG *xptr, LONG *yptr)
{
    if (s->current == 0)
    	return FALSE;
	
    s->current --;
    
    
    *xptr = s->items[s->current].x;
    *yptr = s->items[s->current].y;

    
    return TRUE;
}

static BOOL filline(struct fillinfo *fi, LONG start_x, LONG start_y)
{
    LONG x;
    
    LONG rightmost_above, rightmost_below;
    LONG leftmost_above, leftmost_below;
    struct stack stack;
    
    EnterFunc(bug("filline(fi=%p, start_x=%d, start_y=%d)\n"
    	,fi, start_x, start_y));
	
    init_stack(&stack);
	
    for (;;) {
    /* Scan right */
    
    rightmost_above = start_x;
    rightmost_below = start_x;
    
    for (x = start_x + 1; ; x ++)
    {
    
    	if (fi->isfillable(fi, x, start_y))
	{
	    putfillpixel(fi, x, start_y);
	    
	    /* Check above */
	    if (x > rightmost_above)
	    {
	        if (fi->isfillable(fi, x, start_y - 1))
		{
		    /* Find rightmost pixel */
		    
		    for (rightmost_above = x; ; rightmost_above ++)
		    {
		        if (!fi->isfillable(fi, rightmost_above + 1, start_y - 1))
			    break;
		    }
		    
		    /* Fill that line */
		    if (!push(&stack, rightmost_above, start_y - 1))
		    	ReturnBool("filline (stack full)", FALSE);
/*		    filline(fi, rightmost_above, start_y - 1);
*/		}
		
	    }
	    
	    /* Check below */
	    
	    if (x > rightmost_below)
	    {
	        if (fi->isfillable(fi, x, start_y + 1))
		{
		    /* Find rightmost pixel */
		    
		    for (rightmost_below = x; ; rightmost_below ++)
		    {
		        if (!fi->isfillable(fi, rightmost_below + 1, start_y + 1))
			    break;
		    }
		    
		    /* Fill that line */
		    if (!push(&stack, rightmost_below, start_y + 1))
		    	ReturnBool("filline (stack full)", FALSE);
		    
/*		    filline(fi, rightmost_below, start_y + 1);
*/		}
		
	    }
	    
	}
	else
	    break;

    } /* for (scan right)  */ 
    
    
    /* scan left */
    
    
    leftmost_above = start_x + 1;
    leftmost_below = start_x + 1;

    for (x = start_x; ; x -- )
    {
        
    
    	if (fi->isfillable(fi, x, start_y))
	{
	    putfillpixel(fi, x, start_y);
	    
	    /* Check above */
	    if (x <= leftmost_above)
	    {
	        if (fi->isfillable(fi, x, start_y - 1))
		{
		    /* Find rightmost pixel */
		    
		    for (leftmost_above = x; ; leftmost_above --)
		    {
		        if (!fi->isfillable(fi, leftmost_above - 1, start_y - 1))
			    break;
		    }
		    
		    /* Fill that line */
		    if (!push(&stack, leftmost_above, start_y - 1))
		    	ReturnBool("filline (stack full)", FALSE);
/*		    filline(fi, leftmost_above, start_y - 1);
*/		}
		
	    }
	    
	    /* Check below */
	    
	    if (x < leftmost_below)
	    {
	        if (fi->isfillable(fi, x, start_y + 1))
		{
		    /* Find rightmost pixel */
		    
		    for (leftmost_below = x; ; leftmost_below --)
		    {
		        if (!fi->isfillable(fi, leftmost_below - 1, start_y + 1))
			    break;
		    }
		    
		    /* Fill that line */
		    if (!push(&stack, leftmost_below, start_y + 1))
		       	ReturnBool("filline (stack full)", FALSE);
		    
/*		    filline(fi, leftmost_below, start_y + 1);
*/		}
		
	    }
	    
	}
	else
	    break;

    } /* for (scan left)  */ 

    
    if (!pop(&stack, &start_x, &start_y))
    	break;  
D(bug("\t\t\tpop(%d, %d)\n", start_x, start_y));
    
  } /* forever */

    ReturnBool("filline", TRUE);
    
}


#undef GfxBase

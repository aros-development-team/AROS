/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

#undef SDEBUG
#undef DEBUG

#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

struct fillinfo
{
    ULONG fillpen;
    BOOL (*isfillable)();
    struct RastPort *rp;
    UBYTE *rasptr;
    ULONG bpr;
    struct GfxBase *gfxbase;
};

#if DEBUG
static VOID settmpraspixel(BYTE *rasptr, LONG x, LONG y,  ULONG bpr, UBYTE state);
static BOOL gettmpraspixel(BYTE *rasptr, LONG x, LONG y,  ULONG bpr );
#endif

static BOOL filline(struct fillinfo *fi, LONG start_x, LONG start_y);
static BOOL outline_isfillable(struct fillinfo *fi, LONG x, LONG y);
static BOOL color_isfillable(struct fillinfo *fi, LONG x, LONG y);

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

    INPUTS

    RESULT

    NOTES

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
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    
    struct TmpRas *tmpras = rp->TmpRas;
    ULONG bpr, needed_size;
    ULONG rp_width, rp_height;
    ULONG idx;
    
    struct fillinfo fi;
    
    D(bug("Flood(rp=%p, mode=%d, x=%d, y=%d)\n"
    		, rp, mode, x, y));
    
    /* Check for tmpras */
    if (NULL == tmpras)
    	ReturnBool("Flood (No tmpras)",  FALSE);

    if (NULL != rp->Layer)
    {
    	rp_width  = rp->Layer->Width;
    	rp_height = rp->Layer->Height;
    }
    else
    {
    	rp_width  = GetBitMapAttr(rp->BitMap, BMA_WIDTH);
    	rp_height = GetBitMapAttr(rp->BitMap, BMA_HEIGHT);
    }
    
    bpr = WIDTH_TO_BYTES( rp_width );
    needed_size = bpr * rp_height;
    
    if (tmpras->Size < needed_size)
    	ReturnBool("Flood (To small tmpras)",  FALSE);
    

    /* Clear the needed part of tmpras */

/*    
    
!!! Maybe we should use BltClear() here, since 
!!! tmpras allways reside in CHIP RAM
     */
     
    D(bug("Clearing tmpras\n"));
    memset(tmpras->RasPtr, 0,  needed_size);
    

    D(bug("Drawing outline\n"));
    /* Draw an outline to stop "leaks" */


    D(bug("Left\n"));
    for (idx = 0; idx < bpr; idx ++ )				/* top */
    	tmpras->RasPtr[idx] = 0xFF;
	
    D(bug("Top\n"));
    for (idx = bpr; idx < needed_size; idx += bpr )		/* left */
        tmpras->RasPtr[idx] |= 0x80;

    D(bug("Right\n"));
    for (idx = (bpr * 2) - 1; idx < needed_size; idx += bpr )	/* right */
        tmpras->RasPtr[idx] |= 0x01;
	
    D(bug("Bottom\n"));
    for (idx = bpr * (rp_height - 1); idx < needed_size; idx ++ )	/* bottom */
        tmpras->RasPtr[idx] |= 0xFF;
	
    D(bug("done outlining\n"));
	
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
	D(bug("pixel read\n"));
	fi.isfillable = color_isfillable;
    }
    
    fi.rasptr = tmpras->RasPtr;
    fi.rp = rp;
    fi.bpr = bpr;
    fi.gfxbase = GfxBase;
    
    D(bug("Callinf filline\n"));
    filline(&fi, x, y);
    
    ReturnBool("Flood",  TRUE);
	
    AROS_LIBFUNC_EXIT
} /* Flood */



#if DEBUG

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
    
    if (rasptr[idx] & mask)
	state = TRUE;
    else
    	state = FALSE;
    
    return state;
}

#endif

#undef GfxBase
#define GfxBase (fi->gfxbase)

static BOOL color_isfillable(struct fillinfo *fi, LONG x, LONG y)
{


    BOOL fill;
#if DEBUG
    if (gettmpraspixel(fi->rasptr, x, y, fi->bpr))
    	D(bug("Pixel checked twice at (%d, %d)\n", x, y));

#endif
    
    if (fi->fillpen == ReadPixel(fi->rp, x, y))
    	fill = TRUE;
    else
    	fill = FALSE;
	
    return fill;
       
}

static BOOL outline_isfillable(struct fillinfo *fi, LONG x, LONG y)
{
    BOOL fill;
    EnterFunc(bug("outline_isfillable(fi=%p, x=%d, y=%d)\n",
    	fi, x, y));
    
#if DEBUG
/*    if (gettmpraspixel(fi->rasptr, x, y, fi->bpr))
    	D(bug("Pixel checked twice at (%d, %d)\n", x, y));
*/
#endif
    if (fi->fillpen != ReadPixel(fi->rp, x, y))
    	fill = TRUE;
    else
        fill = FALSE;
	
/*    D(bug("fillpen: %d, pen: %d\n", fi->fillpen, ReadPixel(fi->rp, x, y)));
*/	
	
    ReturnBool("outline_isfillable", fill);
}



static VOID putfillpixel(struct fillinfo *fi, LONG x, LONG y)
{

#warning Implement use of patterns

    WritePixel(fi->rp, x, y);

#if DEBUG
    settmpraspixel(fi->rasptr, x, y, fi->bpr, 1);

#endif    
    return;
}

#define STACKSIZE 1000

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

#if DEBUG
if (x > 100)
	return FALSE;
#endif	        
    
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

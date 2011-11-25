/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: compositingclass.c 38905 2011-05-29 06:54:59Z deadwood $
*/

#define DEBUG 0
#define DTOGGLE(x)
#define DMODE(x)
#define DMOVE(x)
#define DRECALC(x)
#define DREDRAWBM(x)
#define DREDRAWSCR(x)
#define DSTACK(x)
#define DUPDATE(x)

#include "compositing_intern.h"

#include <aros/debug.h>
#include <hidd/graphics_inline.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#define _RECT(x) x.MinX, x.MinY, x.MaxX, x.MaxY

#define MAX(a,b) a > b ? a : b
#define MIN(a,b) a < b ? a : b

static BOOL AndRectRect(struct Rectangle * rect1, struct Rectangle * rect2,
    struct Rectangle * intersect)
{
    intersect->MinX = MAX(rect1->MinX, rect2->MinX);
    intersect->MinY = MAX(rect1->MinY, rect2->MinY);
    intersect->MaxX = MIN(rect1->MaxX, rect2->MaxX);
    intersect->MaxY = MIN(rect1->MaxY, rect2->MaxY);
    
    if ((intersect->MinX > intersect->MaxX) ||
        (intersect->MinY > intersect->MaxY))
        return FALSE;
    else
        return TRUE;
}

static struct StackBitMapNode * HIDDCompositingIsBitMapOnStack(struct HIDDCompositingData * compdata, OOP_Object * bm)
{
    struct StackBitMapNode * n = NULL;
    
    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->bm == bm)
            return n;
    }

    return NULL;
}

static VOID HIDDCompositingValidateBitMapPositionChange(OOP_Object * bm, SIPTR *newxoffset, SIPTR *newyoffset, LONG displayedwidth, LONG displayedheight)
{
    IPTR width, height;
    LONG neglimit, poslimit;

    OOP_GetAttr(bm, aHidd_BitMap_Width, &width);
    OOP_GetAttr(bm, aHidd_BitMap_Height, &height);

    /* Check x position */
    if (width > displayedwidth)
    {
    	neglimit = displayedwidth - width;
    	poslimit = 0;
    }
    else
    {
    	neglimit = 0;
    	poslimit = displayedwidth - width;
    }

    if (*(newxoffset) > poslimit)
        *(newxoffset) = poslimit;
    if (*(newxoffset) < neglimit)
        *(newxoffset) = neglimit;

    /* Check y position */
    if (height > displayedheight)
    	neglimit = displayedheight - height; /* Limit for scroll */
    else
    	neglimit = 0;
    poslimit = displayedheight - 15; /* Limit for drag */

    if (*(newyoffset) > poslimit)
        *(newyoffset) = poslimit;
    if (*(newyoffset) < neglimit)
        *(newyoffset) = neglimit;
}

static VOID HIDDCompositingRecalculateVisibleRects(struct HIDDCompositingData * compdata)
{
    ULONG lastscreenvisibleline = compdata->screenrect.MaxY;
    struct StackBitMapNode * n = NULL;

    DRECALC(bug("[Compositing] Screen rect (%d, %d) - (%d, %d)\n", _RECT(compdata->screenrect)));

    /*
     * This function assumes bitmapstack is in correct Z order: 
     * from topmost to bottom most
     */
    ForeachNode(&compdata->bitmapstack, n)
    {
        /* Get bitmap bounding box in screen coordinates */
        struct Rectangle tmprect;

	tmprect.MinX = n->leftedge;
	tmprect.MaxX = n->leftedge + OOP_GET(n->bm, aHidd_BitMap_Width) - 1;
	tmprect.MinY = n->topedge;
	tmprect.MaxY = n->topedge  + OOP_GET(n->bm, aHidd_BitMap_Height) - 1;

	/* If bitmap's visible portion is smaller, apply this */
	if (lastscreenvisibleline < tmprect.MaxY)
	    tmprect.MaxY = lastscreenvisibleline;

        /* Intersect the box with screen rectangle to make sure values are within screen limit */
        if (AndRectRect(&tmprect, &compdata->screenrect, &n->screenvisiblerect))
        {
            lastscreenvisibleline = n->screenvisiblerect.MinY - 1;
            n->isscreenvisible = TRUE;
        }
        else
            n->isscreenvisible = FALSE;

        DRECALC(bug("[Compositing] Bitmap 0x%x, top %d, visible %d, (%d, %d) - (%d, %d)\n", 
            	    n->bm, n->topedge, n->isscreenvisible, _RECT(n->screenvisiblerect)));
    }
}

static HIDDT_ModeID FindBestHiddMode(struct HIDDCompositingData *compdata, ULONG width, ULONG height, ULONG depth, ULONG *res_depth)
{
    HIDDT_ModeID mode = vHidd_ModeID_Invalid;
    OOP_Object *sync, *pf;
    IPTR w, h, d;
    ULONG dw, dh, delta;
    ULONG found_delta  = -1;
    ULONG found_width  = 0;
    ULONG found_height = 0;
    ULONG found_depth  = 0;
    HIDDT_ModeID found_mode = vHidd_ModeID_Invalid;

    DMODE(bug("[FindBestHiddMode] Looking best maching mode for %u x %u x %u\n", width, height, depth));

    while ((mode = HIDD_Gfx_NextModeID(compdata->gfx, mode, &sync, &pf)) != vHidd_ModeID_Invalid)
    {
    	BOOL match;

    	DMODE(bug("[FindBestHiddMode] Checking mode 0x%08X... ", mode));
	if (OOP_GET(pf, aHidd_PixFmt_ColorModel) != vHidd_ColorModel_TrueColor)
	{
	    DMODE(bug("Skipped (not truecolor)\n"));
	    continue;
	}

	OOP_GetAttr(sync, aHidd_Sync_HDisp, &w);
	OOP_GetAttr(sync, aHidd_Sync_VDisp, &h);
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &d);

	dw = w > width  ? w - width  : w < width  ? width  - w : 1;
	dh = h > height ? h - height : h < height ? height - h : 1;
	delta = dw * dh;

	match = FALSE;
	if (delta < found_delta)
	{
	    /* If mode resolution is closer to the needed one, we've got a better match */
	    found_delta  = delta;
	    found_width  = w;
	    found_height = h;

	    match = TRUE;
	}
	else if (delta == found_delta)
	{
	    /* If resolution is the same as that of current candidate mode, we can look at depth. */
	    if (found_depth > depth)
	    {
	    	/*
	    	 * Candidate mode if deeper than requested. We can supersede it with another mode
	    	 * of smaller depth, but which still matches our request.
	    	 */
	    	if ((d < found_depth) && (d >= depth))
	    	    match = TRUE;
	    }
	    else if (found_depth < depth)
	    {
	    	/*
	    	 * We want better depth than current candidate.
	    	 * In this case anything deeper will do.
	    	 */
	    	if (d > found_depth)
	    	    match = TRUE;
	    }
	}

	if (match)
	{
	    /*
	     * Mode with the same delta, but larger depth, may supersede
	     * previous mode, if we prefer deeper ones.
	     */
	    DMODE(bug("Selected (%ld x %ld x %ld, delta = %u)", w, h, d, delta));
	    found_depth = d;
	    found_mode  = mode;
	}
	DMODE(bug("\n"));
    }

    /* Store mode information */ 
    compdata->screenrect.MinX = 0;
    compdata->screenrect.MinY = 0;
    compdata->screenrect.MaxX = found_width  - 1;
    compdata->screenrect.MaxY = found_height - 1;
    *res_depth = found_depth;

    return found_mode;
}

static void UpdateDisplayMode(struct HIDDCompositingData *compdata)
{
    struct StackBitMapNode *n;
    IPTR modeid, width, height, depth;
    OOP_Object *sync, *pf;
    UBYTE comp_depth = 16;
    ULONG found_depth;

    /*
     * Examine all bitmaps in the stack to figure out the needed depth.
     * We need a maximum depth of all depths in order to keep correct colors.
     * But not less than 16 bits, because we can't compose on a LUT screen.
     *
     * If a LUT bitmap is present in the stack (depth < 9), we request truecolor
     * screen for better color presentation.
     *
     * We examine bitmaps in reverse order, in this case 'sync' will hold
     * information about the top bitmap when we exit the loop.
     * Size of our composited mode needs to be as close as possible to that one.
     */
    for (n = (struct StackBitMapNode *)compdata->bitmapstack.mlh_TailPred;
    	 n->n.mln_Pred; n = (struct StackBitMapNode *)n->n.mln_Pred)
    {
	OOP_GetAttr(n->bm, aHidd_BitMap_ModeID, &modeid);
	HIDD_Gfx_GetMode(compdata->gfx, modeid, &sync, &pf);

	if (OOP_GET(pf, aHidd_PixFmt_ColorModel) == vHidd_ColorModel_TrueColor)
	{
	    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	    if (depth > comp_depth)
	    	comp_depth = depth;
	}
	else
	{
	    /*
	     * If we have a LUT bitmap on stack, we request 24-bit screen
	     * for better color transfer.
	     */
	    comp_depth = 24;
	}
    }

    /* Get the needed size */
    OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
    OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);

    DSTACK(bug("[UpdateDisplayMode] Requested mode %ld x %ld x %d\n", width, height, comp_depth));

    modeid = FindBestHiddMode(compdata, width, height, depth, &found_depth);
    DSTACK(bug("[UpdateDisplayMode] Composition mode 0x%08X, current 0x%08X\n", modeid, compdata->screenmodeid));

    if (modeid != compdata->screenmodeid)
    {
    	/* The mode is different. Need to prepare information needed for compositing */
        struct TagItem gctags[] =
        {
            { aHidd_GC_Foreground, 0x99999999 }, 
            { TAG_DONE		 , 0	      }
        };

        /* Signal mode change */ 
        compdata->screenmodeid = modeid;
        compdata->modeschanged = TRUE;

        /* Get gray foregound */
        if (found_depth < 24)
            gctags[0].ti_Data = 0x9492;

        OOP_SetAttrs(compdata->gc, gctags);
    }
}

static inline void HIDDCompositingRedrawBitmap(struct HIDDCompositingData *compdata, struct StackBitMapNode *n, struct Rectangle *rect)
{
    /* The given rectangle is already in screen coordinate system here */
    ULONG blitwidth  = rect->MaxX - rect->MinX + 1;
    ULONG blitheight = rect->MaxY - rect->MinY + 1;

    DREDRAWBM(bug("[Compositing] Redraw bitmap 0x%p, rect (%d, %d) - (%d, %d)\n", n->bm,
    		  rect->MinX, rect->MinY, rect->MaxX, rect->MaxY));
    DREDRAWBM(bug("[Compositing] Blitting %d x %d from (%d, %d)\n", blitwidth, blitheight, 
		  rect->MinX - n->leftedge, rect->MinY - n->topedge));

    HIDD_Gfx_CopyBox(compdata->gfx, n->bm,
                    /* Transform to source bitmap coord system */
                    rect->MinX - n->leftedge, rect->MinY - n->topedge,
                    compdata->compositedbitmap,
                    rect->MinX, rect->MinY, blitwidth, blitheight,
                    compdata->gc);
}

static inline void ClearRect(struct HIDDCompositingData *compdata, ULONG MinX, ULONG MinY, ULONG MaxX, ULONG MaxY)
{
    DREDRAWSCR(bug("[Compositing] Clearing area (%d, %d) - (%d, %d)\n",
		   MinX, MinY, MaxX, MaxY));

    HIDD_BM_FillRect(compdata->compositedbitmap, compdata->gc,
		     MinX, MinY, MaxX, MaxY);
}

static VOID HIDDCompositingRedrawVisibleScreen(struct HIDDCompositingData * compdata)
{
    struct StackBitMapNode *n;
    ULONG lastscreenvisibleline = 0;

    DREDRAWSCR(bug("[Compositing] Redrawing screen\n"));

    /* Calculations are performed regardless if compositedbitmap is beeing show.
       Gfx operations are only performed if compositedbitmap is beeing show */
    
    /* Recalculate visible rects per screen */
    HIDDCompositingRecalculateVisibleRects(compdata);
    
    /*
     * Refresh all bitmaps on stack.
     * For simplicity, we traverse the list in reverse order (from bottom to top).
     * This way we can easy figure out spaces between bitmaps (every time we know
     * final line of previous bitmap or 0 if there was no one).
     */
    for (n = (struct StackBitMapNode *)compdata->bitmapstack.mlh_TailPred;
    	 n->n.mln_Pred; n = (struct StackBitMapNode *)n->n.mln_Pred)
    {
	DREDRAWSCR(bug("[Compositing] Bitmap 0x%p, visible %d, rect (%d, %d) - (%d, %d)\n",
		       n->bm, n->isscreenvisible, _RECT(n->screenvisiblerect)));

        if (n->isscreenvisible)
        {
	    /* Redraw the whole visible portion */
            HIDDCompositingRedrawBitmap(compdata, n, &n->screenvisiblerect);

	    /* Clean up areas revealed by drag */
	    if (n->screenvisiblerect.MinX > 0)
	    {
	    	/* To the left of bitmap */
	    	ClearRect(compdata,
	    		  0, n->screenvisiblerect.MinY,
			  n->screenvisiblerect.MinX, n->screenvisiblerect.MaxY);
	    }

	    if (n->screenvisiblerect.MaxX < compdata->screenrect.MaxX)
	    {
	    	/* To the right of bitmap */
		ClearRect(compdata,
			  n->screenvisiblerect.MaxX + 1, n->screenvisiblerect.MinY,
			  compdata->screenrect.MaxX, n->screenvisiblerect.MaxY);
	    }

	    if (n->screenvisiblerect.MinY > lastscreenvisibleline)
	    {
	    	/* Between this and previous bitmap */
	    	ClearRect(compdata,
	    		  0, lastscreenvisibleline,
			  compdata->screenrect.MaxX, n->screenvisiblerect.MinY - 1);
	    }

	    /* Update the whole display rectangle (bitmap + space around it) in one operation */
	    HIDD_BM_UpdateRect(compdata->compositedbitmap,
	        	       0, lastscreenvisibleline,
	        	       compdata->screenrect.MaxX + 1, n->screenvisiblerect.MaxY - lastscreenvisibleline + 1);

            lastscreenvisibleline = n->screenvisiblerect.MaxY + 1;
        }
    }
}

/*  

There are several cases that needs to be handled in this code. They are documented
below. Please read it before making changes.
etb     = existing topbitmap
ntb     = new top bitmap
sb      = screen bitmap
cb      = composited bitmap
fs      = covers full screen
nfs     = not covers full screen
mA      = mode "A"
mB      = mode "B"
disp()  = dispose
new()   = new

The resulting mode is always that of screen bitmap as set in "effect" column.
The composited bitmap always matches the resulting screen mode or is NULL.

| exiting screen situation          | change         | effect                               |
| USE CASE: SWITCHING BETWEEN FULL SCREEN SCREENS - SAME MODE                               |
| etb->fs, mA, sb==etb, cb==NULL    | ntb->fs, mA    | sb==ntb, cb==NULL                    |
| etb->fs, mA, sb==etb, cb!=NULL    | ntb->fs, mA    | sb==ntb, cb!=NULL                    |
| USE CASE: SWITCHING BETWEEN FULL SCREEN AND NOT FULL SCREEN SCREENS - SAME MODE           |
| etb->fs, mA, sb==etb, cb==NULL    | ntb->nfs, mA   | new(cb), sb==cb, cb!=NULL            |
| etb->fs, mA, sb==etb, cb!=NULL    | ntb->nfs, mA   | sb==cb, cb!=NULL                     |
| USE CASE: SWITCHING BETWEEN NOT FULL SCREEN AND FULL SCREEN SCREENS - SAME MODE           |
| etb->nfs, mA, sb==cb, cb==NULL    | NOT POSSIBLE                                          |
| etb->nfs, mA, sb==cb, cb!=NULL    | ntb->fs, mA    | sb==ntb, cb!=NULL                    |
| USE CASE: SWITCHING BETWEEN NOT FULL SCREEN AND NOT FULL SCREEN SCREENS - SAME MODE       |
| etb->nfs, mA, sb==cb, cb==NULL    | NOT POSSIBLE                                          |
| etb->nfs, mA, sb==cb, cb!=NULL    | ntb->nfs, mA   | sb==cb, cb!=NULL                     |


| USE CASE: SWITCHING BETWEEN FULL SCREEN SCREENS - DIFFERENT MODES                         |
| etb->fs, mA, sb==etb, cb==NULL    | ntb->fs, mB    | sb==ntb, cb==NULL                    |
| etb->fs, mA, sb==etb, cb!=NULL    | ntb->fs, mB    | disp(cb), sb==ntb, cb==NULL          |
| USE CASE: SWITCHING BETWEEN FULL SCREEN AND NOT FULL SCREEN SCREENS - DIFFERENT MODES     |
| etb->fs, mA, sb==etb, cb==NULL    | ntb->nfs, mB   | new(cb), sb==cb, cb!=NULL            |
| etb->fs, mA, sb==etb, cb!=NULL    | ntb->nfs, mB   | disp(cb), new(cb), sb==cb, cb!=NULL  |
| USE CASE: SWITCHING BETWEEN NOT FULL SCREEN AND FULL SCREEN SCREENS - DIFFERENT MODES     |
| etb->nfs, mA, sb==cb, cb==NULL    | NOT POSSIBLE                                          |
| etb->nfs, mA, sb==cb, cb!=NULL    | ntb->fs, mB    | disp(cb), sb==ntb, cb==NULL          |
| USE CASE: SWITCHING BETWEEN NOT FULL SCREEN AND NOT FULL SCREEN SCREENS - DIFFERENT MODES |
| etb->nfs, mA, sb==cb, cb==NULL    | NOT POSSIBLE                                          |
| etb->nfs, mA, sb==cb, cb!=NULL    | ntb->nfs, mB   | disp(cb), new(cb), sb==cb, cb!=NULL  |


| USE CASE: DRAGGING SCREEN DOWN                                                            |
| etb->fs, mA, sb==etb, cb==NULL    | ntb->nfs, mA   | new(cb), sb==cb                      |
| etb->fs, mA, sb==etb, cb!=NULL    | ntb->nfs, mA   | sb==cb                               |
| USE CASE: DRAGGING SCREEN UP                                                              |
| etb->nfs, mA, sb==cb, cb!=NULL    | ntb->fs, mA    | sb==etb                              |
| etb->nfs, mA, sb==cb, cb==NULL    | NOT POSSIBLE                                          |

Resulting rules (order matters):

(a) if ((cb!=NULL) && (etb->mode!=ntb->mode)) {dispose(cb), cb=NULL}
(b) if ((ntb->nfs) && (cb==NULL)) new(cb)
(c) if (ntb->nfs) sb=cb
(d) if (ntb->fs) sb=ntb

Additional rule:
(e) if (oldsb!=sb) modeswitch(sb)

02.09.2011: we don't remember sb any more because we don't handle it in any way. Instead
	    we either have or don't have compositedbitmap. If we have it, composition
	    is on (sb = cb). If we don't have it, composition is off (sb = ntb).
*/
static BOOL HIDDCompositingToggleCompositing(struct HIDDCompositingData *compdata, BOOL newtop)
{
    /* 
     * If the topbitmap covers the complete screen, show it instead of 
     * compositedbitmap. Remember that screen bitmap -> composited bitmap
     * mirroring has a negative impact on performance.
     */
    OOP_Object *oldcompositedbitmap = compdata->compositedbitmap;
    OOP_Object *newscreenbitmap = NULL;
    LONG topedge = ((struct StackBitMapNode *)compdata->bitmapstack.mlh_Head)->topedge;
    BOOL ok = TRUE;

    /* (a) If mode change is needed, enforce opening a new screen */
    if (compdata->modeschanged)
    {
    	D(bug("[Compositing] Mode changed\n"));
        compdata->compositedbitmap = NULL;
    }

    /*
     * This condition is enough as compositing allows only dragging screen down
     * and not up/left/right at the moment.
     */
    if (topedge > 0)
    {
        /* (b) */
        if (compdata->compositedbitmap == NULL)
        {
            /*
             * compositedbitmap == NULL means we were in passthrough mode before.
             * Set up screen for composition.
	     */
	    if (compdata->fb)
	    {
	    	/*
	    	 * If our display driver uses a framebuffer, we can reuse it.
	    	 * Copy its original contents back into the bitmap which it replaced,
	    	 * then change framebuffer's video mode.
	    	 * Framebuffer is the only bitmap which can change its ModeID on the fly.
	    	 */
	    	D(bug("[Compositing] Using framebuffer bitmap 0x%p\n", compdata->fb));

	    	 /* Do this comparison in order not to show the framebuffer twice */
	    	if (oldcompositedbitmap != compdata->fb)
	    	{
	    	    /*
	    	     * 1. It's legal to show the framebuffer itself. This causes copying
	    	     *	  back old bitmap contents and detaching from it.
	    	     * 2. The result of this will always match compdata->fb.
	    	     * 3. Internally this is a simple blit operation, it can't fail.
	    	     */
	    	    compdata->screenbitmap = HIDD_Gfx_Show(compdata->gfx, compdata->fb, fHidd_Gfx_Show_CopyBack);
	    	}

		/* Switch display mode on the framebuffer. */
	    	OOP_SetAttrsTags(compdata->fb, aHidd_BitMap_ModeID, compdata->screenmodeid, TAG_DONE);
		/* We are now compositing on the framebuffer */
	    	compdata->compositedbitmap = compdata->fb;
	    }
	    else
	    {
	        /*
	         * There's no framebuffer.
	         * Create a new bitmap that will be used for compositing.
	         */
            	struct TagItem bmtags[5];

            	bmtags[0].ti_Tag = aHidd_BitMap_Width;          bmtags[0].ti_Data = compdata->screenrect.MaxX + 1;
            	bmtags[1].ti_Tag = aHidd_BitMap_Height;         bmtags[1].ti_Data = compdata->screenrect.MaxY + 1;
            	bmtags[2].ti_Tag = aHidd_BitMap_Displayable;    bmtags[2].ti_Data = TRUE;
            	bmtags[3].ti_Tag = aHidd_BitMap_ModeID;         bmtags[3].ti_Data = compdata->screenmodeid;
            	bmtags[4].ti_Tag = TAG_DONE;                    bmtags[4].ti_Data = TAG_DONE;

            	compdata->compositedbitmap = HIDD_Gfx_NewBitMap(compdata->gfx, bmtags);
            	D(bug("[Compositing] Created working bitmap 0x%p\n", compdata->compositedbitmap));

		/* Mode changed, this bitmap will be shown later */
		newscreenbitmap = compdata->compositedbitmap;

		/* NewBitMap can fail, handle this */
		if (!newscreenbitmap)
		    ok = FALSE;
            }
        }
        else /* if (compdata->compositedbitmap == NULL) */
        {
            /*
             * We are already in compositing mode and will stay in it.
             * Do not destroy our working bitmap.
             */
            oldcompositedbitmap = NULL;
        }

        /*
         * (c) Here composition is turned on (compositedbitmap != NULL).
         * Redraw bitmap stack - compensate possible changes
         */
        if (ok)
            HIDDCompositingRedrawVisibleScreen(compdata);
    }
    else if (oldcompositedbitmap || newtop)
    {
        /*
         * (d) Set passthrough mode and display the frontmost bitmap.
         * This is also triggered by 'newtop' parameter, which tells us
         * that frontmost bitmap has been changed, and we need to display a new one.
         * Old compositedbitmap has been remembered in the beginning. If it's not
         * NULL, it will be destroyed in the end.
         */
        newscreenbitmap = compdata->topbitmap;
        compdata->compositedbitmap = NULL;
    }

    DTOGGLE(bug("[Compositing] Toggle te %d, oldcomp 0x%P, top 0x%P, comp 0x%P, newscreen 0x%P\n",
            topedge, oldcompositedbitmap, compdata->topbitmap, compdata->compositedbitmap, newscreenbitmap));

    /*
     * (e) If the screenbitmap changed, show the new screenbitmap.
     * We do it after refreshing, for better visual appearance.
     */
    if (newscreenbitmap)
    {
    	IPTR w, h;

    	compdata->screenbitmap = HIDD_Gfx_Show(compdata->gfx, newscreenbitmap, fHidd_Gfx_Show_CopyBack);
    	D(bug("[Compositing] Displayed bitmap 0x%p, Show returned 0x%p\n", newscreenbitmap, compdata->screenbitmap));

	/* After Show we need Update for mirroring drivers */
	if (compdata->screenbitmap)
        {
            OOP_GetAttr(compdata->screenbitmap, aHidd_BitMap_Width, &w);
            OOP_GetAttr(compdata->screenbitmap, aHidd_BitMap_Height, &h);
            HIDD_BM_UpdateRect(compdata->screenbitmap, 0, 0, w, h);
        }
    }

    /*
     * (a) - disposing of oldcompositingbitmap needs to happen after mode switch 
     * since it could have been the current screenbitmap
     */
    if (oldcompositedbitmap && (oldcompositedbitmap != compdata->fb))
    {
    	D(bug("[Compositing] Disposing old working bitmap 0x%p\n", oldcompositedbitmap));
        HIDD_Gfx_DisposeBitMap(compdata->gfx, oldcompositedbitmap);
    }

    /* Handled */
    compdata->modeschanged = FALSE;

    return ok;
}

static VOID HIDDCompositingPurgeBitMapStack(struct HIDDCompositingData * compdata)
{
    struct StackBitMapNode * curr, * next;

    ForeachNodeSafe(&compdata->bitmapstack, curr, next)
    {
        FreeMem(curr, sizeof(struct StackBitMapNode));
    }

    NEWLIST(&compdata->bitmapstack);
}

static void HIDDCompositingShowSingle(struct HIDDCompositingData *compdata, OOP_Object *bm)
{
    /* Show the single top bitmap */
    compdata->topbitmap = bm;
    compdata->screenbitmap = HIDD_Gfx_Show(compdata->gfx, bm, fHidd_Gfx_Show_CopyBack);

    /* Dispose working bitmap (if any) */
    if (compdata->compositedbitmap)
    {
    	/* Be careful with the framebuffer */
    	if (compdata->compositedbitmap != compdata->fb)
    	    HIDD_Gfx_DisposeBitMap(compdata->gfx, compdata->compositedbitmap);

	/* This will deactivate us */
	compdata->compositedbitmap = NULL;
    }
}

/* Emergency error recovery function */
static void HIDDCompositingReset(struct HIDDCompositingData *compdata)
{
    /* Purge bitmap stack */
    HIDDCompositingPurgeBitMapStack(compdata);

    /*
     * Reset our internal state so that next BitMapStackChanged
     * causes complete reinitialization.
     */
    compdata->screenmodeid = vHidd_ModeID_Invalid;
    compdata->screenbitmap = NULL;
}

/* PUBLIC METHODS */
OOP_Object *METHOD(Compositing, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if (o)
    {
        OOP_MethodID disposemid;
        struct HIDDCompositingData *compdata = OOP_INST_DATA(cl, o);
        
        NEWLIST(&compdata->bitmapstack);
        compdata->screenmodeid = vHidd_ModeID_Invalid;
        InitSemaphore(&compdata->semaphore);

        compdata->gfx = (OOP_Object *)GetTagData(aHidd_Compositing_GfxHidd, 0, msg->attrList);
        compdata->fb  = (OOP_Object *)GetTagData(aHidd_Compositing_FrameBuffer, 0, msg->attrList);

	/* GfxHidd is mandatory */
        if (compdata->gfx != NULL)
        {
            /* Create GC object that will be used for drawing operations */
            compdata->gc = HIDD_Gfx_NewGC(compdata->gfx, NULL);

            if (compdata->gfx)
            	return o;
        }

        /* Creation failed */

        disposemid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, &disposemid);
    }

    return NULL;
}

VOID METHOD(Compositing, Root, Get)
{
    if (msg->attrID == aHidd_Compositing_Capabilities)
    {
	*msg->storage = COMPF_ABOVE;
	return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

OOP_Object *METHOD(Compositing, Hidd_Compositing, BitMapStackChanged)
{
    struct HIDD_ViewPortData *vpdata;
    struct HIDDCompositingData *compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode *n;
    BOOL newtop = FALSE;
    BOOL ok = TRUE;

    DSTACK(bug("[BitMapStackChanged] Top bitmap: 0x%lx\n", msg->data->Bitmap));

    LOCK_COMPOSITING_WRITE

    /* Free all items which are already on the list */
    HIDDCompositingPurgeBitMapStack(compdata);

    if (!msg->data)
    {
        UNLOCK_COMPOSITING

	/* Blank screen */
	HIDDCompositingShowSingle(compdata, NULL);

	/* We know we are inactive after this */
	*msg->active = FALSE;
        /* This can return NULL, it's okay */
        return compdata->screenbitmap;
    }

    /* Copy bitmaps pointers to our stack */
    for (vpdata = msg->data; vpdata; vpdata = vpdata->Next)
    {
        n = AllocMem(sizeof(struct StackBitMapNode), MEMF_ANY | MEMF_CLEAR);
	if (!n)
	{
	    /*
	     * Error happened.
	     * We need to reset own state and return NULL. graphics.library
	     * falls back to no composition in this case.
	     */
	    DSTACK(bug("[BitMapStackChanged] Error allocating StackBitMapNode!!!\n"));

	    ok = FALSE;
	    break;
	}

	DSTACK(bug("[BitMapStackChanged] ViewPort 0x%p, offset (%d, %d)\n", vpdata->vpe->ViewPort, vpdata->vpe->ViewPort->DxOffset, vpdata->vpe->ViewPort->DyOffset));

        n->bm              = vpdata->Bitmap;
        n->isscreenvisible = FALSE;
        n->leftedge	       = vpdata->vpe->ViewPort->DxOffset;
        n->topedge	       = vpdata->vpe->ViewPort->DyOffset;

        AddTail((struct List *)&compdata->bitmapstack, (struct Node *)n);
    }

    /* Switch mode if needed */
    UpdateDisplayMode(compdata);

    if (msg->data->Bitmap != compdata->topbitmap)
    {
    	/* Set the new pointer to top bitmap */
    	compdata->topbitmap = msg->data->Bitmap;
    	newtop = TRUE;
    }

    if (ok)
    {
    	/*
    	 * Validate bitmap offsets - they might not match the compositing rules taking
       	 * new displayedwidth/displayedheight values
       	 */
	ForeachNode(&compdata->bitmapstack, n)
    	{
            HIDDCompositingValidateBitMapPositionChange(n->bm, &n->leftedge, &n->topedge, 
            						compdata->screenrect.MaxX + 1, compdata->screenrect.MaxY + 1);
            DSTACK(bug("[BitMapStackChanged] Bitmap 0x%p, display size %d x %d, validated position (%ld, %ld)\n",
            	       n->bm, compdata->screenrect.MaxX + 1, compdata->screenrect.MaxY + 1,
            	       n->leftedge, n->topedge));
    	}

    	/* Toogle compositing based on screen positions */
    	ok = HIDDCompositingToggleCompositing(compdata, newtop);
    }

    /* Handle error */
    if (!ok)
    {
    	HIDDCompositingReset(compdata);
    	HIDDCompositingShowSingle(compdata, msg->data->Bitmap);
    }

    UNLOCK_COMPOSITING

    DSTACK(bug("[BitMapStackChanged] Done, composited bitmap 0x%p\n", compdata->compositedbitmap));

    /* Tell if the composition is active */
    *msg->active = compdata->compositedbitmap ? TRUE : FALSE;
    /* Return actually displayed bitmap */
    return compdata->screenbitmap;
}

VOID METHOD(Compositing, Hidd_Compositing, BitMapRectChanged)
{
    struct HIDDCompositingData * compdata = OOP_INST_DATA(cl, o);

    if (compdata->compositedbitmap)
    {
	/* Composition is active, handle redraw if the bitmap is on screen */
    	struct StackBitMapNode *n;

	DUPDATE(bug("[BitMapRectChanged] Bitmap 0x%p\n", msg->bm));

    	LOCK_COMPOSITING_READ

	n = HIDDCompositingIsBitMapOnStack(compdata, msg->bm);
	if (n && n->isscreenvisible)
	{
	    /* Rectangle in bitmap coord system */
    	    struct Rectangle srcrect, dstandvisrect;

	    srcrect.MinX = msg->x; 
	    srcrect.MinY = msg->y;
	    srcrect.MaxX = msg->x + msg->width - 1; 
	    srcrect.MaxY = msg->y + msg->height - 1;
    	    DUPDATE(bug("[BitMapRectChanged] Bitmap rect (%d, %d) - (%d, %d)\n", _RECT(srcrect)));

	    /* Transform the rectangle to screen coord system */
	    srcrect.MinX += n->leftedge; 
	    srcrect.MaxX += n->leftedge;
	    srcrect.MinY += n->topedge;
	    srcrect.MaxY += n->topedge;
	    DUPDATE(bug("[BitMapRectChanged] Screen-relative rect (%d, %d) - (%d, %d)\n", _RECT(srcrect)));

    	    /* Find intersection of visible screen rect and bitmap rect */
    	    if (AndRectRect(&srcrect, &n->screenvisiblerect, &dstandvisrect))
    	    {
    	        /* Intersection is valid. Blit. */
    	    	DUPDATE(bug("[BitMapRectChanged] Clipped rect (%d, %d) - (%d, %d)\n", _RECT(dstandvisrect)));

    	    	HIDDCompositingRedrawBitmap(compdata, n, &dstandvisrect);

    		HIDD_BM_UpdateRect(compdata->compositedbitmap,
    				   dstandvisrect.MinX, dstandvisrect.MinY,
    				   dstandvisrect.MaxX - dstandvisrect.MinX + 1,
				   dstandvisrect.MaxY - dstandvisrect.MinY + 1);
    	    }
    	}

    	UNLOCK_COMPOSITING
    	
    	DUPDATE(bug("[BitMapRectChanged] Done\n"));
    }
    else
    {
	/*
	 * In order to speed things up, we handle passthrough ourselves here.
	 * It's not difficult.
	 */
    	HIDD_BM_UpdateRect(msg->bm, msg->x, msg->y, msg->width, msg->height);
    }
}

IPTR METHOD(Compositing, Hidd_Compositing, BitMapPositionChange)
{
    struct HIDDCompositingData *compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode *n;
    IPTR disp_width, disp_height;

    LOCK_COMPOSITING_READ

    n = HIDDCompositingIsBitMapOnStack(compdata, msg->bm);
    if (n)
    {
    	/* The bitmap is on display. Validate against screen size */
        disp_width  = compdata->screenrect.MaxX + 1;
        disp_height = compdata->screenrect.MaxY + 1;
    }
    else
    {
	/* The bitmap is not displayed yet. Validate against its own ModeID size. */
    	HIDDT_ModeID modeid = vHidd_ModeID_Invalid;
    	OOP_Object *sync, *pf;

    	OOP_GetAttr(msg->bm, aHidd_BitMap_ModeID, &modeid);

    	if (modeid == vHidd_ModeID_Invalid)
    	{
    	    /*
    	     * Nondisplayable bitmaps don't scroll.
    	     * In fact they simply can't get in here because MakeVPort() performs the validation.
    	     * But who knows what bug can slip into someone's software...
    	     */
    	    UNLOCK_COMPOSITING
    	    return FALSE;
    	}

    	HIDD_Gfx_GetMode(compdata->gfx, modeid, &sync, &pf);
    	OOP_GetAttr(sync, aHidd_Sync_HDisp, &disp_width);
    	OOP_GetAttr(sync, aHidd_Sync_VDisp, &disp_height);
    }

    DMOVE(bug("[BitMapPositionChange] Validating bitmap 0x%p, position (%ld, %ld), limits %ld x %ld\n",
    	  msg->bm, *msg->newxoffset, *msg->newyoffset, disp_width, disp_height));

    HIDDCompositingValidateBitMapPositionChange(msg->bm, msg->newxoffset, msg->newyoffset,
    						disp_width, disp_height);

    DMOVE(bug("[BitMapPositionChange] Validated position (%ld, %ld)\n", *msg->newxoffset, *msg->newyoffset));

    if (n && ((*msg->newxoffset != n->leftedge) || (*msg->newyoffset != n->topedge)))
    {
    	DMOVE(bug("[BitMapPositionChange] Old position (%ld, %ld)\n", n->leftedge, n->topedge));

	/* Reflect the change if it happened */
    	n->leftedge = *msg->newxoffset;
    	n->topedge  = *msg->newyoffset;

    	if (compdata->topbitmap == msg->bm)
    	{
    	    /*
    	     * If this is the frontmost bitmap, we may want to toggle compositing,
    	     * if it starts/stops covering the whole screen at one point.
    	     * We don't need to call HIDDCompositingRedrawVisibleScreen() here because
    	     * HIDDCompositingToggleCompositing() does this itself, for improved
    	     * visual appearance.
    	     */
            HIDDCompositingToggleCompositing(compdata, FALSE);    
        }
        else
            HIDDCompositingRedrawVisibleScreen(compdata);
    }

    UNLOCK_COMPOSITING

    /* Return active state */
    return compdata->compositedbitmap ? TRUE : FALSE;
}

#define NUM_Compositing_Root_METHODS 2

static const struct OOP_MethodDescr Compositing_Root_descr[] =
{
    {(OOP_MethodFunc)Compositing__Root__New, moRoot_New},
    {(OOP_MethodFunc)Compositing__Root__Get, moRoot_Get},
    {NULL, 0}
};

#define NUM_Compositing_Hidd_Compositing_METHODS 3

static const struct OOP_MethodDescr Compositing_Hidd_Compositing_descr[] =
{
    {(OOP_MethodFunc)Compositing__Hidd_Compositing__BitMapStackChanged, moHidd_Compositing_BitMapStackChanged},
    {(OOP_MethodFunc)Compositing__Hidd_Compositing__BitMapRectChanged, moHidd_Compositing_BitMapRectChanged},
    {(OOP_MethodFunc)Compositing__Hidd_Compositing__BitMapPositionChange, moHidd_Compositing_BitMapPositionChange},
    {NULL, 0}
};

const struct OOP_InterfaceDescr Compositing_ifdescr[] =
{
    {Compositing_Root_descr, IID_Root, NUM_Compositing_Root_METHODS},
    {Compositing_Hidd_Compositing_descr, IID_Hidd_Compositing, NUM_Compositing_Hidd_Compositing_METHODS},
    {NULL, NULL}
};

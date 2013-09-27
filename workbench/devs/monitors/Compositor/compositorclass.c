/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#if (DEBUG)
#define DTOGGLE(x) x
#define DMODE(x) x
#define DMOVE(x) x
#define DRECALC(x) x
#define DREDRAWBM(x) x
#define DREDRAWSCR(x) x
#define DSTACK(x) x
#define DUPDATE(x) x
#else
#define DTOGGLE(x)
#define DMODE(x)
#define DMOVE(x)
#define DRECALC(x)
#define DREDRAWBM(x)
#define DREDRAWSCR(x)
#define DSTACK(x)
#define DUPDATE(x)
#endif

#include <aros/debug.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <graphics/view.h>
#include <hidd/graphics.h>

#include "compositor_intern.h"

#define COMPOSITOR_PREFS "SYS/compositor.prefs"
#define COMPOSITOR_PEFSTEMPLATE  "ABOVE/S,BELOW/S,LEFT/S,RIGHT/S,ALPHA/S"

enum
{
    ARG_ABOVE = 0,
    ARG_BELOW,
    ARG_LEFT,
    ARG_RIGHT,
    ARG_ALPHA,
    NOOFARGS
};


#ifdef GfxBase
#undef GfxBase
#endif
#define GfxBase compdata->GraphicsBase

#define _RECT(x) x.MinX, x.MinY, x.MaxX, x.MaxY

#define MAX(a,b) a > b ? a : b
#define MIN(a,b) a < b ? a : b

static struct StackBitMapNode * HIDDCompositorIsBitMapOnStack(struct HIDDCompositorData * compdata, OOP_Object * bm)
{
    struct StackBitMapNode * n = NULL;
    
    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->bm == bm)
            return n;
    }

    return NULL;
}

static VOID HIDDCompositorValidateBitMapPositionChange(OOP_Object * bm, SIPTR *newxoffset, SIPTR *newyoffset, LONG displayedwidth, LONG displayedheight)
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

static VOID HIDDCompositorRecalculateVisibleRegions(struct HIDDCompositorData *compdata)
{
    struct StackBitMapNode      *n = NULL, *tmpn;
    struct Region               *dispvisregion = NULL;
    OOP_Object                  *bmpxfmt;

    DRECALC(bug("[Compositor:%s] Display rect [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__, _RECT(compdata->screenrect)));

    /*
     * This function assumes bitmapstack is in correct Z order: 
     * from topmost to bottom most
     */
    if ((dispvisregion = NewRegion()) != NULL)
    {
        OrRectRegion(dispvisregion, &compdata->screenrect);
        
        DRECALC(bug("[Compositor:%s] DisplayRegion @ 0x%p\n", __PRETTY_FUNCTION__, dispvisregion));

        ForeachNodeSafe(&compdata->bitmapstack, n, tmpn)
        {
            /* Get bitmap bounding box in screen coordinates */
            struct Rectangle tmprect;

            n->sbmflags &= ~STACKNODEF_VISIBLE;

            if (n->screenregion)
                ClearRegion(n->screenregion);
            else
                n->screenregion = NewRegion();

            if (n->screenregion)
            {
                tmprect.MinX = n->leftedge;
                tmprect.MaxX = n->leftedge + OOP_GET(n->bm, aHidd_BitMap_Width) - 1;
                tmprect.MinY = n->topedge;
                tmprect.MaxY = n->topedge  + OOP_GET(n->bm, aHidd_BitMap_Height) - 1;

                DRECALC(bug("[Compositor:%s] Screen Rect [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__, _RECT(&tmprect)));

                OrRectRegion(n->screenregion, &tmprect); // Start with the Screen's dimensions ..
                AndRegionRegion(dispvisregion, n->screenregion); // And adjust for the "Display"

                if (n->screenregion->RegionRectangle)
                {
                    int bmpstdfmt;
                    bmpxfmt = (OOP_Object *)OOP_GET(n->bm, aHidd_BitMap_PixFmt);
                    bmpstdfmt = (int)OOP_GET(bmpxfmt, aHidd_PixFmt_StdPixFmt);
                    DRECALC(bug("[Compositor:%s] Screen BitMap PixFmt %lx @ 0x%p\n", __PRETTY_FUNCTION__, bmpstdfmt, bmpxfmt));

                    switch (bmpstdfmt)
                    {
                        case vHidd_StdPixFmt_ARGB32:
                        case vHidd_StdPixFmt_BGRA32:
                        case vHidd_StdPixFmt_RGBA32:
                        case vHidd_StdPixFmt_ABGR32:
                        {
                            DRECALC(bug("[Compositor:%s] BitMap has ALPHA\n", __PRETTY_FUNCTION__));

                            n->sbmflags |= STACKNODEF_HASALPHA;

                            if (!((compdata->flags & COMPSTATEF_HASALPHA)))
                                compdata->flags |= COMPSTATEF_HASALPHA;

                            break;
                        }
                        default:
                        {
                            n->sbmflags &= ~STACKNODEF_HASALPHA;

                            AndRectRect(&n->screenregion->bounds, &compdata->screenrect, &n->screenvisiblerect);
                            ClearRegionRegion(n->screenregion, dispvisregion);

                            if (!(compdata->capabilities & COMPF_ABOVE))
                            {
                                tmprect.MinX = compdata->screenrect.MinX;
                                tmprect.MaxX = compdata->screenrect.MaxX;
                                tmprect.MinY = compdata->screenrect.MinY;
                                tmprect.MaxY = n->topedge - 1;
                                ClearRectRegion(dispvisregion, &tmprect);
                            }
                            if (!(compdata->capabilities & COMPF_BELOW))
                            {
                                tmprect.MinX = compdata->screenrect.MinX;
                                tmprect.MaxX = compdata->screenrect.MaxX;
                                tmprect.MinY = n->topedge  + OOP_GET(n->bm, aHidd_BitMap_Height);
                                tmprect.MaxY = compdata->screenrect.MaxY;
                                ClearRectRegion(dispvisregion, &tmprect);
                            }
                            if (!(compdata->capabilities & COMPF_LEFT))
                            {
                                tmprect.MinX = compdata->screenrect.MinX;
                                tmprect.MaxX = n->leftedge - 1;
                                tmprect.MinY = n->topedge;
                                tmprect.MaxY = n->topedge  + OOP_GET(n->bm, aHidd_BitMap_Height) - 1;
                                ClearRectRegion(dispvisregion, &tmprect);
                            }
                            if (!(compdata->capabilities & COMPF_RIGHT))
                            {
                                tmprect.MinX = n->leftedge + OOP_GET(n->bm, aHidd_BitMap_Width);
                                tmprect.MaxX = compdata->screenrect.MaxX;
                                tmprect.MinY = n->topedge;
                                tmprect.MaxY = n->topedge  + OOP_GET(n->bm, aHidd_BitMap_Height) - 1;
                                ClearRectRegion(dispvisregion, &tmprect);
                            }
                        }
                    }
                    n->sbmflags |= STACKNODEF_VISIBLE;
                }

                DRECALC(bug("[Compositor:%s] Bitmap 0x%x, topedge %d, visible %d, [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__, 
                            n->bm, n->topedge, (n->sbmflags & STACKNODEF_VISIBLE), _RECT(n->screenvisiblerect)));
            }
            else
            {
                DRECALC(bug("[Compositor:%s] Failed to create Screen Region\n", __PRETTY_FUNCTION__));
            }
        }
        DisposeRegion(dispvisregion);
    }
    else
    {
        DRECALC(bug("[Compositor:%s] Failed to create Display Region\n", __PRETTY_FUNCTION__));
    }
}

static HIDDT_ModeID FindBestHiddMode(struct HIDDCompositorData *compdata, ULONG width, ULONG height, ULONG depth, ULONG *res_depth)
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

    DMODE(bug("[%s] Finding best match for mode %ux%ux%u\n", __PRETTY_FUNCTION__, width, height, depth));

    while ((mode = HIDD_Gfx_NextModeID(compdata->gfx, mode, &sync, &pf)) != vHidd_ModeID_Invalid)
    {
    	BOOL match;

    	DMODE(bug("[%s] Checking mode 0x%08X... ", __PRETTY_FUNCTION__, mode));
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
	    DMODE(bug("Selected (%ldx%ldx%ld, delta = %u)", w, h, d, delta));
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

static void UpdateDisplayMode(struct HIDDCompositorData *compdata)
{
    struct StackBitMapNode *n;
    IPTR modeid, width, height, depth;
    OOP_Object *sync, *pf;
    UBYTE comp_depth = 16;
    ULONG found_depth;

    compdata->flags &= ~COMPSTATEF_HASALPHA;

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

    DSTACK(bug("[%s] Requested mode %ldx%ldx%d\n", __PRETTY_FUNCTION__, width, height, comp_depth));

    modeid = FindBestHiddMode(compdata, width, height, depth, &found_depth);
    DSTACK(bug("[%s] Composition Display Mode 0x%08X [current 0x%08X]\n", __PRETTY_FUNCTION__, modeid, compdata->screenmodeid));

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

static inline void HIDDCompositorRedrawBitmap(struct HIDDCompositorData *compdata, struct StackBitMapNode *n, struct Rectangle *rect)
{
    /* The given rectangle is already in screen coordinate system here */
    ULONG blitwidth  = rect->MaxX - rect->MinX + 1;
    ULONG blitheight = rect->MaxY - rect->MinY + 1;

    DREDRAWBM(bug("[Compositor:%s] Redraw BitMap 0x%p, Rect[%d, %d - %d, %d]\n", __PRETTY_FUNCTION__, n->bm,
    		  rect->MinX, rect->MinY, rect->MaxX, rect->MaxY));
    DREDRAWBM(bug("[Compositor:%s] Blitting %dx%d [from %d, %d]\n", __PRETTY_FUNCTION__, blitwidth, blitheight, 
		  rect->MinX - n->leftedge, rect->MinY - n->topedge));

    if (!(n->sbmflags & STACKNODEF_HASALPHA))
    {
        HIDD_Gfx_CopyBox(compdata->gfx, n->bm,
                        /* Transform to source bitmap coord system */
                        rect->MinX - n->leftedge, rect->MinY - n->topedge,
                        compdata->compositedbitmap,
                        rect->MinX, rect->MinY, blitwidth, blitheight,
                        compdata->gc);
    }
    else
    {
        //TODO: Alpha blend bitmap..
    }
}

static inline void ClearRect(struct HIDDCompositorData *compdata, ULONG MinX, ULONG MinY, ULONG MaxX, ULONG MaxY)
{
    DREDRAWSCR(bug("[Compositor:%s] Clearing [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__,
		   MinX, MinY, MaxX, MaxY));

    HIDD_BM_FillRect(compdata->compositedbitmap, compdata->gc,
		     MinX, MinY, MaxX, MaxY);
}

static VOID HIDDCompositorRedrawVisibleDisplay(struct HIDDCompositorData *compdata)
{
    struct Region       *dispvisregion = NULL;
    struct Rectangle    tmprect;

    struct StackBitMapNode *n;

    DREDRAWSCR(bug("[Compositor:%s] Redrawing screen (GfxBase @ 0x%p)\n", __PRETTY_FUNCTION__, GfxBase));

    /* Recalculate visible regions */
    HIDDCompositorRecalculateVisibleRegions(compdata);

    if ((dispvisregion = NewRegion()) != NULL)
    {
        OrRectRegion(dispvisregion, &compdata->screenrect);
        DRECALC(bug("[Compositor:%s] Display rect [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__, _RECT(compdata->screenrect)));

        ForeachNode(&compdata->bitmapstack, n)
        {
            if ((n->sbmflags & STACKNODEF_VISIBLE) &&
                (!(n->sbmflags & STACKNODEF_HASALPHA)) &&
                (n->screenregion))
            {
                DREDRAWSCR(bug("[Compositor:%s] ScreenRegion @ 0x%p ScreenBitMap @ 0x%p [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__,
                               n->screenregion, n->bm, _RECT(n->screenvisiblerect)));

                DREDRAWSCR(bug("[Compositor:%s] Redrawing Visible Screen Regions..\n", __PRETTY_FUNCTION__));
                // Render the visable regions ..
                struct RegionRectangle * srrect = n->screenregion->RegionRectangle;
                while (srrect)
                {
                    tmprect.MinX = srrect->bounds.MinX + n->screenregion->bounds.MinX;
                    tmprect.MinY = srrect->bounds.MinY + n->screenregion->bounds.MinY;
                    tmprect.MaxX = srrect->bounds.MaxX + n->screenregion->bounds.MinX;
                    tmprect.MaxY = srrect->bounds.MaxY + n->screenregion->bounds.MinY;

                    DREDRAWSCR(bug("[Compositor:%s] Region [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__, _RECT(&tmprect)));

                    HIDDCompositorRedrawBitmap(compdata, n, &tmprect);
                    HIDD_BM_UpdateRect(compdata->compositedbitmap,
                       tmprect.MinX, tmprect.MinY,
                       tmprect.MaxX - tmprect.MinX + 1,
                       tmprect.MaxY - tmprect.MinY + 1);

                    srrect = srrect->Next;
                }
                ClearRegionRegion(n->screenregion, dispvisregion);
            }
        }
        struct RegionRectangle * dispclrrect = dispvisregion->RegionRectangle;
        while (dispclrrect)
        {
            struct HIDD_BackFillHookMsg clearmsg;

            tmprect.MinX = dispclrrect->bounds.MinX + dispvisregion->bounds.MinX;
            tmprect.MinY = dispclrrect->bounds.MinY + dispvisregion->bounds.MinY;
            tmprect.MaxX = dispclrrect->bounds.MaxX + dispvisregion->bounds.MinX;
            tmprect.MaxY = dispclrrect->bounds.MaxY + dispvisregion->bounds.MinY;

            DREDRAWSCR(bug("[Compositor:%s] Render Display Void Region [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__, _RECT(&tmprect)));

            clearmsg.bounds = &tmprect;
            clearmsg.offsetx = 0;
            clearmsg.offsety = 0;
            CallHookPkt(compdata->backfillhook, compdata->compositedbitmap, &clearmsg);
            HIDD_BM_UpdateRect(compdata->compositedbitmap,
               tmprect.MinX, tmprect.MinY,
               tmprect.MaxX - tmprect.MinX + 1,
               tmprect.MaxY - tmprect.MinY + 1);

            dispclrrect = dispclrrect->Next;
        }
        if (compdata->flags & COMPSTATEF_HASALPHA)
        {
            ForeachNode(&compdata->bitmapstack, n)
            {
                if ((n->sbmflags & STACKNODEF_VISIBLE) &&
                    (n->sbmflags & STACKNODEF_HASALPHA) &&
                    (n->screenregion))
                {
                    DREDRAWSCR(bug("[Compositor:%s] Alpha-ScreenRegion @ 0x%p ScreenBitMap @ 0x%p [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__,
                                       n->screenregion, n->bm, _RECT(n->screenvisiblerect)));
                    
                    DREDRAWSCR(bug("[Compositor:%s] Compositing Visible Alpha Regions..\n", __PRETTY_FUNCTION__));

                    struct RegionRectangle * srrect = n->screenregion->RegionRectangle;
                    while (srrect)
                    {
                        tmprect.MinX = srrect->bounds.MinX + n->screenregion->bounds.MinX;
                        tmprect.MinY = srrect->bounds.MinY + n->screenregion->bounds.MinY;
                        tmprect.MaxX = srrect->bounds.MaxX + n->screenregion->bounds.MinX;
                        tmprect.MaxY = srrect->bounds.MaxY + n->screenregion->bounds.MinY;

                        DREDRAWSCR(bug("[Compositor:%s] Alpha-Region [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__, _RECT(&tmprect)));

                        HIDDCompositorRedrawBitmap(compdata, n, &tmprect);
                        HIDD_BM_UpdateRect(compdata->compositedbitmap,
                           tmprect.MinX, tmprect.MinY,
                           tmprect.MaxX - tmprect.MinX + 1,
                           tmprect.MaxY - tmprect.MinY + 1);

                        srrect = srrect->Next;
                    }
                }
            }
        }
        DisposeRegion(dispvisregion);
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
static BOOL HIDDCompositorToggleCompositing(struct HIDDCompositorData *compdata, BOOL newtop)
{
    /* 
     * If the topbitmap covers the complete screen, show it instead of 
     * compositedbitmap. Remember that screen bitmap -> composited bitmap
     * mirroring has a negative impact on performance.
     */
    OOP_Object *oldcompositedbitmap = compdata->compositedbitmap;
    OOP_Object *newscreenbitmap = NULL;
    BOOL ok = TRUE;

    /* (a) If mode change is needed, enforce opening a new screen */
    if (compdata->modeschanged)
    {
    	D(bug("[Compositor:%s] Display Mode changed\n", __PRETTY_FUNCTION__));
        compdata->compositedbitmap = NULL;
    }

#if (0)
    /*
     * This condition is enough as compositing allows only dragging screen down
     * and not up/left/right at the moment.
     */
    if (topedge > 0)
    {
#endif
        /* (b) */
        if (compdata->compositedbitmap == NULL)
        {
            /*
             * compositedbitmap == NULL means we were in passthrough mode before,
             * or have just changed display mode - set up screen for composition.
	     */
            D(bug("[Compositor:%s] Initialising Display-Compositor..\n", __PRETTY_FUNCTION__));

	    if (compdata->fb)
	    {
	    	/*
	    	 * If our display driver uses a framebuffer, we can reuse it.
	    	 * Copy its original contents back into the bitmap which it replaced,
	    	 * then change framebuffer's video mode.
	    	 * Framebuffer is the only bitmap which can change its ModeID on the fly.
	    	 */
	    	D(bug("[Compositor:%s] Using Display Famebuffer BitMap @ 0x%p\n", __PRETTY_FUNCTION__, compdata->fb));

	    	 /* Do this comparison in order not to show the framebuffer twice */
	    	if (oldcompositedbitmap != compdata->fb)
	    	{
	    	    /*
	    	     * 1. It's legal to show the framebuffer itself. This causes copying
	    	     *	  back old bitmap contents and detaching from it.
	    	     * 2. The result of this will always match compdata->fb.
	    	     * 3. Internally this is a simple blit operation, it can't fail.
	    	     */
                    D(bug("[Compositor:%s] Copying old Famebuffer BitMap\n", __PRETTY_FUNCTION__));
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
            	D(bug("[Compositor:%s] Created Compositor Display BitMap @ 0x%p\n", __PRETTY_FUNCTION__, compdata->compositedbitmap));

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
            HIDDCompositorRedrawVisibleDisplay(compdata);
#if(0)
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
#endif

    DTOGGLE(bug("[Compositor:%s] oldcompbmp 0x%p, topbmp 0x%p, compbmp 0x%p, newscreenbmp 0x%p\n", __PRETTY_FUNCTION__,
            oldcompositedbitmap, compdata->topbitmap, compdata->compositedbitmap, newscreenbitmap));

    /*
     * (e) If the screenbitmap changed, show the new screenbitmap.
     * We do it after refreshing, for better visual appearance.
     */
    if (newscreenbitmap)
    {
    	IPTR w, h;

    	compdata->screenbitmap = HIDD_Gfx_Show(compdata->gfx, newscreenbitmap, fHidd_Gfx_Show_CopyBack);
    	D(bug("[Compositor:%s] Displayed bitmap 0x%p, Show returned 0x%p\n", __PRETTY_FUNCTION__, newscreenbitmap, compdata->screenbitmap));

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
    	D(bug("[Compositor:%s] Disposing old working bitmap 0x%p\n", __PRETTY_FUNCTION__, oldcompositedbitmap));
        HIDD_Gfx_DisposeBitMap(compdata->gfx, oldcompositedbitmap);
    }

    /* Handled */
    compdata->modeschanged = FALSE;

    return ok;
}

static VOID HIDDCompositorPurgeBitMapStack(struct HIDDCompositorData * compdata)
{
    struct StackBitMapNode * curr, * next;

    ForeachNodeSafe(&compdata->bitmapstack, curr, next)
    {
        if (curr->screenregion)
            DisposeRegion(curr->screenregion);

        FreeMem(curr, sizeof(struct StackBitMapNode));
    }

    NEWLIST(&compdata->bitmapstack);
}

static void HIDDCompositorShowSingle(struct HIDDCompositorData *compdata, OOP_Object *bm)
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
static void HIDDCompositorReset(struct HIDDCompositorData *compdata)
{
    /* Purge bitmap stack */
    HIDDCompositorPurgeBitMapStack(compdata);

    /*
     * Reset our internal state so that next BitMapStackChanged
     * causes complete reinitialization.
     */
    compdata->screenmodeid = vHidd_ModeID_Invalid;
    compdata->screenbitmap = NULL;
}

VOID CompositorParseConfig(struct HIDDCompositorData *compdata)
{
    struct RDArgs *rdargs;
    IPTR CompArgs[NOOFARGS] = { 0 };
    TEXT CompConfig[1024];

    /* use default amiga-like capabailities */
    compdata->capabilities = COMPF_ABOVE;

    rdargs = AllocDosObjectTags(DOS_RDARGS, TAG_END);
    if (rdargs != NULL)
    {
        if (GetVar(COMPOSITOR_PREFS, CompConfig, 1024, GVF_GLOBAL_ONLY) != -1)
        {
            rdargs->RDA_Source.CS_Buffer = CompConfig;
            rdargs->RDA_Source.CS_Length = strlen(rdargs->RDA_Source.CS_Buffer);
            rdargs->RDA_DAList = (IPTR)NULL;
            rdargs->RDA_Buffer = NULL;
            rdargs->RDA_BufSiz = 0;
            rdargs->RDA_ExtHelp = NULL;
            rdargs->RDA_Flags = 0;

            if (ReadArgs(COMPOSITOR_PEFSTEMPLATE, CompArgs, rdargs) != NULL)
            {
                if (CompArgs[ARG_ABOVE])
                    compdata->capabilities |= COMPF_ABOVE;
                else
                    compdata->capabilities &= ~COMPF_ABOVE;
                
                if (CompArgs[ARG_BELOW])
                    compdata->capabilities |= COMPF_BELOW;
                else
                    compdata->capabilities &= ~COMPF_BELOW;
                
                if (CompArgs[ARG_LEFT])
                    compdata->capabilities |= COMPF_LEFT;
                else
                    compdata->capabilities &= ~COMPF_LEFT;
                
                if (CompArgs[ARG_RIGHT])
                    compdata->capabilities |= COMPF_RIGHT;
                else
                    compdata->capabilities &= ~COMPF_RIGHT;
    /*
                if (CompArgs[ARG_ALPHA])
                    compdata->capabilities |= COMPF_ALPHA;
                else
                    compdata->capabilities &= ~COMPF_ALPHA;
    */
                FreeArgs(rdargs);
            }
        }
        FreeDosObject(DOS_RDARGS, rdargs);
    }
}

AROS_UFH3(void, CompositorDefaultBackFillFunc,
    AROS_UFHA(struct Hook *             , h,      A0),
    AROS_UFHA(OOP_Object *              , bm,     A2),
    AROS_UFHA(struct HIDD_BackFillHookMsg *  , msg,    A1))
{
    AROS_USERFUNC_INIT

    struct HIDDCompositorData *compdata = h->h_Data;

    D(bug("[%s] HIDDCompositorData @ 0x%p\n", __PRETTY_FUNCTION__, compdata));

    ClearRect(compdata, msg->bounds->MinX, msg->bounds->MinY, msg->bounds->MaxX, msg->bounds->MaxY);

    AROS_USERFUNC_EXIT
}

/* PUBLIC METHODS */
OOP_Object *METHOD(Compositor, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if (o)
    {
        OOP_MethodID disposemid;
        struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);

        D(bug("[%s] Compositor @ 0x%p, data @ 0x%p\n", __PRETTY_FUNCTION__, o, compdata));

        CompositorParseConfig(compdata);

        D(bug("[%s] Composite Capabilities: %08lx\n", __PRETTY_FUNCTION__, compdata->capabilities));

        compdata->screenmodeid = vHidd_ModeID_Invalid;

        NEWLIST(&compdata->bitmapstack);

        compdata->defaultbackfill.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(CompositorDefaultBackFillFunc);
        compdata->defaultbackfill.h_Data = compdata;
        compdata->backfillhook = &compdata->defaultbackfill;

        InitSemaphore(&compdata->semaphore);

        compdata->gfx = (OOP_Object *)GetTagData(aHidd_Compositor_GfxHidd, 0, msg->attrList);
        compdata->fb  = (OOP_Object *)GetTagData(aHidd_Compositor_FrameBuffer, 0, msg->attrList);

        compdata->GraphicsBase = (APTR)OpenLibrary("graphics.library", 41);

	/* GfxHidd is mandatory */
        if ((compdata->GraphicsBase) && (compdata->gfx != NULL))
        {
            /* Create GC object that will be used for drawing operations */
            compdata->gc = HIDD_Gfx_NewGC(compdata->gfx, NULL);

            D(bug("[%s] Compositor GC @ %p\n", __PRETTY_FUNCTION__, compdata->gc));

            if ((compdata->gfx) && (compdata->gc))
            	return o;
        }

        /* Creation failed */
        disposemid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, &disposemid);
    }

    return NULL;
}

void METHOD(Compositor, Root, Dispose)
{

    D(
        struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
        bug("[%s] HIDDCompositorData @ 0x%p\n", __PRETTY_FUNCTION__, compdata);
      )

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

VOID METHOD(Compositor, Root, Get)
{
    ULONG idx;

    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);

    if (IS_COMPOSITOR_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
            case aoHidd_Compositor_Capabilities:
            {
                D(bug("[%s] Composite Capabilities: %lx\n", __PRETTY_FUNCTION__, compdata->capabilities));
                *msg->storage = (IPTR)COMPF_ABOVE|COMPF_BELOW|COMPF_LEFT|COMPF_RIGHT;
                return;
            }
            case aoHidd_Compositor_BackFillHook:
            {
                D(bug("[%s] BackFillHook: 0x%p\n", __PRETTY_FUNCTION__, compdata->backfillhook));
                *msg->storage = (IPTR)compdata->backfillhook;
                return;
            }
	}
    }
    OOP_DoSuperMethod(cl, o, &msg->mID);
}

VOID METHOD(Compositor, Root, Set)
{
    ULONG idx;

    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    struct TagItem *tag, *tstate = msg->attrList;

    while ((tag = NextTagItem(&tstate)))
    {
        if (IS_COMPOSITOR_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
                case aoHidd_Compositor_Capabilities:
                {
                    D(bug("[%s] Composite Capabilities: %lx -> %lx\n", __PRETTY_FUNCTION__, compdata->capabilities, tag->ti_Data));
                    compdata->capabilities = (ULONG)tag->ti_Data;
                    break;
                }
                case aoHidd_Compositor_BackFillHook:
                {
                    if (tag->ti_Data)
                    {
                        D(bug("[%s] BackFillHook: 0x%p -> 0x%p\n", __PRETTY_FUNCTION__, compdata->backfillhook, tag->ti_Data));
                        compdata->backfillhook = (struct Hook *)tag->ti_Data;
                    }
                    else
                    {
                        D(bug("[%s] Default BackFillHook\n", __PRETTY_FUNCTION__));
                        compdata->backfillhook = &compdata->defaultbackfill;
                    }
                    break;
                }
            }
        }
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

OOP_Object *METHOD(Compositor, Hidd_Compositor, BitMapStackChanged)
{
    struct HIDD_ViewPortData *vpdata;
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode *n;
    BOOL newtop = FALSE;
    BOOL ok = TRUE;

    DSTACK(bug("[%s] Top bitmap: 0x%lx\n", __PRETTY_FUNCTION__, msg->data->Bitmap));

    LOCK_COMPOSITOR_WRITE

    /* Free all items which are already on the list */
    HIDDCompositorPurgeBitMapStack(compdata);

    if (!msg->data)
    {
        UNLOCK_COMPOSITOR

	/* Blank screen */
	HIDDCompositorShowSingle(compdata, NULL);

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
	    DSTACK(bug("[%s] Error allocating StackBitMapNode!!!\n", __PRETTY_FUNCTION__));

	    ok = FALSE;
	    break;
	}

	DSTACK(bug("[%s] ViewPort 0x%p, offset (%d, %d)\n", __PRETTY_FUNCTION__, vpdata->vpe->ViewPort, vpdata->vpe->ViewPort->DxOffset, vpdata->vpe->ViewPort->DyOffset));

        n->bm                   = vpdata->Bitmap;
        n->sbmflags             = 0;
        n->leftedge             = vpdata->vpe->ViewPort->DxOffset;
        n->topedge              = vpdata->vpe->ViewPort->DyOffset;

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
            HIDDCompositorValidateBitMapPositionChange(n->bm, &n->leftedge, &n->topedge, 
            						compdata->screenrect.MaxX + 1, compdata->screenrect.MaxY + 1);
            DSTACK(bug("[%s] Bitmap 0x%p, display size %d x %d, validated position (%ld, %ld)\n", __PRETTY_FUNCTION__,
            	       n->bm, compdata->screenrect.MaxX + 1, compdata->screenrect.MaxY + 1,
            	       n->leftedge, n->topedge));
    	}

    	/* Toogle compositing based on screen positions */
    	ok = HIDDCompositorToggleCompositing(compdata, newtop);
    }

    /* Handle error */
    if (!ok)
    {
    	HIDDCompositorReset(compdata);
    	HIDDCompositorShowSingle(compdata, msg->data->Bitmap);
    }

    UNLOCK_COMPOSITOR

    DSTACK(bug("[%s] Done, composited bitmap 0x%p\n", __PRETTY_FUNCTION__, compdata->compositedbitmap));

    /* Tell if the composition is active */
    *msg->active = compdata->compositedbitmap ? TRUE : FALSE;
    /* Return actually displayed bitmap */
    return compdata->screenbitmap;
}

VOID METHOD(Compositor, Hidd_Compositor, BitMapRectChanged)
{
    struct HIDDCompositorData * compdata = OOP_INST_DATA(cl, o);

    if (compdata->compositedbitmap)
    {
	/* Composition is active, handle redraw if the bitmap is on screen */
    	struct StackBitMapNode *n;

	DUPDATE(bug("[%s] Bitmap 0x%p\n", __PRETTY_FUNCTION__, msg->bm));

    	LOCK_COMPOSITOR_READ

	n = HIDDCompositorIsBitMapOnStack(compdata, msg->bm);
	if (n && (n->sbmflags & STACKNODEF_VISIBLE))
	{
    	    struct Rectangle srcrect, dstandvisrect;

	    srcrect.MinX = n->leftedge + msg->x;
	    srcrect.MinY = n->topedge + msg->y;
	    srcrect.MaxX = n->leftedge + msg->x + msg->width - 1; 
	    srcrect.MaxY = n->topedge + msg->y + msg->height - 1;
    	    DUPDATE(bug("[%s] Bitmap rect [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__, msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1));

	    DUPDATE(bug("[%s] Screen-relative rect [%d, %d - %d, %d]\n", __PRETTY_FUNCTION__, _RECT(srcrect)));

            struct RegionRectangle * srrect = n->screenregion->RegionRectangle;
            while (srrect)
            {
                dstandvisrect.MinX = srrect->bounds.MinX + n->screenregion->bounds.MinX;
                dstandvisrect.MinY = srrect->bounds.MinY + n->screenregion->bounds.MinY;
                dstandvisrect.MaxX = srrect->bounds.MaxX + n->screenregion->bounds.MinX;
                dstandvisrect.MaxY = srrect->bounds.MaxY + n->screenregion->bounds.MinY;

                if (AndRectRect(&srcrect, &dstandvisrect, &dstandvisrect))
                {
                    /* Intersection is valid. Blit. */
                    DUPDATE(bug("[%s] Clipped rect (%d, %d) - (%d, %d)\n", __PRETTY_FUNCTION__, _RECT(dstandvisrect)));

                    HIDDCompositorRedrawBitmap(compdata, n, &dstandvisrect);

                    HIDD_BM_UpdateRect(compdata->compositedbitmap,
                                       dstandvisrect.MinX, dstandvisrect.MinY,
                                       dstandvisrect.MaxX - dstandvisrect.MinX + 1,
                                       dstandvisrect.MaxY - dstandvisrect.MinY + 1);
                }
                srrect = srrect->Next;
            }
     	}

    	UNLOCK_COMPOSITOR
    	
    	DUPDATE(bug("[%s] Done\n", __PRETTY_FUNCTION__));
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

IPTR METHOD(Compositor, Hidd_Compositor, BitMapPositionChange)
{
    struct HIDDCompositorData *compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode *n;
    IPTR disp_width, disp_height;

    LOCK_COMPOSITOR_READ

    n = HIDDCompositorIsBitMapOnStack(compdata, msg->bm);
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
    	    UNLOCK_COMPOSITOR
    	    return FALSE;
    	}

    	HIDD_Gfx_GetMode(compdata->gfx, modeid, &sync, &pf);
    	OOP_GetAttr(sync, aHidd_Sync_HDisp, &disp_width);
    	OOP_GetAttr(sync, aHidd_Sync_VDisp, &disp_height);
    }

    DMOVE(bug("[%s] Validating bitmap 0x%p, position (%ld, %ld), limits %ld x %ld\n", __PRETTY_FUNCTION__,
    	  msg->bm, *msg->newxoffset, *msg->newyoffset, disp_width, disp_height));

    HIDDCompositorValidateBitMapPositionChange(msg->bm, msg->newxoffset, msg->newyoffset,
    						disp_width, disp_height);

    DMOVE(bug("[%s] Validated position (%ld, %ld)\n", __PRETTY_FUNCTION__, *msg->newxoffset, *msg->newyoffset));

    if (n && ((*msg->newxoffset != n->leftedge) || (*msg->newyoffset != n->topedge)))
    {
    	DMOVE(bug("[%s] Old position (%ld, %ld)\n", __PRETTY_FUNCTION__, n->leftedge, n->topedge));

	/* Reflect the change if it happened */
    	n->leftedge = *msg->newxoffset;
    	n->topedge  = *msg->newyoffset;

    	if (compdata->topbitmap == msg->bm)
    	{
    	    /*
    	     * If this is the frontmost bitmap, we may want to toggle compositing,
    	     * if it starts/stops covering the whole screen at one point.
    	     * We don't need to call HIDDCompositorRedrawVisibleDisplay() here because
    	     * HIDDCompositorToggleCompositing() does this itself, for improved
    	     * visual appearance.
    	     */
            HIDDCompositorToggleCompositing(compdata, FALSE);    
        }
        else
            HIDDCompositorRedrawVisibleDisplay(compdata);
    }

    UNLOCK_COMPOSITOR

    /* Return active state */
    return compdata->compositedbitmap ? TRUE : FALSE;
}

#define NUM_Compositor_Root_METHODS 4

static const struct OOP_MethodDescr Compositor_Root_descr[] =
{
    {(OOP_MethodFunc)Compositor__Root__New, moRoot_New},
    {(OOP_MethodFunc)Compositor__Root__Dispose, moRoot_Dispose},
    {(OOP_MethodFunc)Compositor__Root__Get, moRoot_Get},
    {(OOP_MethodFunc)Compositor__Root__Set, moRoot_Set},
    {NULL, 0}
};

#define NUM_Compositor_Hidd_Compositor_METHODS 3

static const struct OOP_MethodDescr Compositor_Hidd_Compositor_descr[] =
{
    {(OOP_MethodFunc)Compositor__Hidd_Compositor__BitMapStackChanged, moHidd_Compositor_BitMapStackChanged},
    {(OOP_MethodFunc)Compositor__Hidd_Compositor__BitMapRectChanged, moHidd_Compositor_BitMapRectChanged},
    {(OOP_MethodFunc)Compositor__Hidd_Compositor__BitMapPositionChange, moHidd_Compositor_BitMapPositionChange},
    {NULL, 0}
};

const struct OOP_InterfaceDescr Compositor_ifdescr[] =
{
    {Compositor_Root_descr, IID_Root, NUM_Compositor_Root_METHODS},
    {Compositor_Hidd_Compositor_descr, IID_Hidd_Compositor, NUM_Compositor_Hidd_Compositor_METHODS},
    {NULL, NULL}
};

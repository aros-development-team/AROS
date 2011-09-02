/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: compositingclass.c 38905 2011-05-29 06:54:59Z deadwood $
*/

#define DEBUG 1

#include "compositing_intern.h"

#include <aros/debug.h>
#include <hidd/graphics_inline.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

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
    LONG limit;

    OOP_GetAttr(bm, aHidd_BitMap_Width, &width);
    OOP_GetAttr(bm, aHidd_BitMap_Height, &height);

    /* Check x position */
    limit = displayedwidth - width;
    if (*(newxoffset) > 0)
        *(newxoffset) = 0;

    if (*(newxoffset) < limit)
        *(newxoffset) = limit;

    /* Check y position */
    limit = displayedheight - height;
    if (*(newyoffset) > displayedheight - 15) /* Limit for drag */
        *(newyoffset) = displayedheight - 15;

    if (*(newyoffset) < limit) /* Limit for scroll */
        *(newyoffset) = limit;
}


static VOID HIDDCompositingRecalculateVisibleRects(struct HIDDCompositingData * compdata)
{
    ULONG lastscreenvisibleline = compdata->screenrect.MaxY;
    struct StackBitMapNode * n = NULL;
    
    /* This function assumes bitmapstack is in correct Z order: 
       from top most to bottom most */
    
    ForeachNode(&compdata->bitmapstack, n)
    {
        /*  Stack bitmap bounding boxes equal screen bounding box taking into
            account topedge */
        struct Rectangle tmprect;
        
        /* Copy screen rect */
        tmprect = compdata->screenrect;
        /* Set bottom and top values */
        tmprect.MinY = n->topedge;
        tmprect.MaxY = lastscreenvisibleline;
        /* Intersect both to make sure values are withint screen limit */
        if (AndRectRect(&tmprect, &compdata->screenrect, &n->screenvisiblerect))
        {
            lastscreenvisibleline = n->screenvisiblerect.MinY - 1;
            n->isscreenvisible = TRUE;
        }
        else
            n->isscreenvisible = FALSE;

        D(bug("[Compositing] Bitmap 0x%x, visible %d, (%d, %d) , (%d, %d)\n", 
            n->bm, n->isscreenvisible, 
            n->screenvisiblerect.MinX, n->screenvisiblerect.MinY, 
            n->screenvisiblerect.MaxX, n->screenvisiblerect.MaxY));
    }
}

static BOOL HIDDCompositingTopBitMapChanged(struct HIDDCompositingData *compdata, OOP_Object *bm)
{
    IPTR modeid;

    if (bm == compdata->topbitmap)
    {
    	/* Nothing changed here */
    	return FALSE;
    }

    /* Set the pointer to top bitmap */
    compdata->topbitmap = bm;

    /* Read display mode properties */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);

    if (modeid != compdata->screenmodeid)
    {
    	/* The mode is different. Need to prepare information needed for compositing */
	OOP_Object *sync, *pf;
        struct TagItem gctags[] =
        {
            { aHidd_GC_Foreground, 0x99999999 }, 
            { TAG_DONE		 , 0	      }
        };

	HIDD_Gfx_GetMode(compdata->gfx, modeid, &sync, &pf);

        /* Store mode information */ 
        compdata->screenmodeid      = modeid;
        compdata->screenrect.MinX   = 0;
        compdata->screenrect.MinY   = 0;
        compdata->screenrect.MaxX   = OOP_GET(sync, aHidd_Sync_HDisp) - 1;
        compdata->screenrect.MaxY   = OOP_GET(sync, aHidd_Sync_VDisp) - 1;
        compdata->modeschanged      = TRUE;

        /* Get gray foregound */
        if (OOP_GET(pf, aHidd_PixFmt_Depth) < 24)
            gctags[0].ti_Data = 0x9492;

        OOP_SetAttrs(compdata->gc, gctags);
    }

    return TRUE;
}

static BOOL HIDDCompositingCanCompositeWithScreenBitMap(struct HIDDCompositingData * compdata, OOP_Object * bm)
{
    OOP_Object * screenbm = compdata->topbitmap; /* Tread top bitmap as screen bitmap */
    IPTR screenbmwidth, screenbmheight, screenbmstdpixfmt;
    IPTR bmmodeid, bmwidth, bmheight, bmstdpixfmt;

    {
        IPTR pf;
        /* These two values cannot be obtained from topbitmap width/height, because
           those can be larger than screen mode values and we are interested exaclty
           in screen mode height/width */
        screenbmwidth   = compdata->screenrect.MaxX + 1;
        screenbmheight  = compdata->screenrect.MaxY + 1;
        OOP_GetAttr(screenbm, aHidd_BitMap_PixFmt, &pf);
        OOP_GetAttr((OOP_Object*)pf, aHidd_PixFmt_StdPixFmt, &screenbmstdpixfmt);
    }

    {
        IPTR pf;

        OOP_GetAttr(bm, aHidd_BitMap_ModeID, &bmmodeid);
        OOP_GetAttr(bm, aHidd_BitMap_Width, &bmwidth);
        OOP_GetAttr(bm, aHidd_BitMap_Height, &bmheight);
        OOP_GetAttr(bm, aHidd_BitMap_PixFmt, &pf);
        OOP_GetAttr((OOP_Object*)pf, aHidd_PixFmt_StdPixFmt, &bmstdpixfmt);
    }

    /* If bitmaps have the same modeid, they can be composited */
    if (compdata->screenmodeid == bmmodeid)
        return TRUE;

    /* If bitmaps have different pixel formats, they cannot be composited */
    /* FIXME: actually they can, but CopyBox for different formats is not
       optimized so let's not make user experience worse */
    if (screenbmstdpixfmt != bmstdpixfmt)
        return FALSE;

    /* If screenbm is not bigger than bm, bitmaps can be composited, because
       bm will start scrolling on smaller resolution of screenbm */
    /* FIXME: the opposite situation might also work - smaller bitmap on bigger
       resolution. In such case, left out areas need to be cleared up. Also it
       needs to be checked if mouse clicks outside of bitmap will not cause
       problems */
    if ((screenbmwidth <= bmwidth) && (screenbmheight <= bmheight))
        return TRUE;

    /* Last decision, bitmaps cannot be composited */
    return FALSE;
}

static VOID HIDDCompositingRedrawBitmap(struct HIDDCompositingData * compdata,
    OOP_Object * bm, WORD x, WORD y, WORD width, WORD height)
{
    /* We do our work only if composition is active */
    if (compdata->compositedbitmap)
    {
        struct Rectangle srcrect;
        struct Rectangle srcindstrect;
        struct Rectangle dstandvisrect;
    	struct StackBitMapNode *n = HIDDCompositingIsBitMapOnStack(compdata, bm);
    
    	/*
    	 * Skip the bitmap if:
    	 * 1. It' not on stack (not displayed at the moment)
    	 * 2. It's completely covered with another bitmap
    	 */
    	if ((!n) || (!n->isscreenvisible))
            return;

        /* Rectangle in source bitmap coord system */
        srcrect.MinX = x; 
        srcrect.MinY = y;
        srcrect.MaxX = x + width - 1; 
        srcrect.MaxY = y + height - 1;
        
        /* Source bitmap rectangle in destination (screen) coord system */
        srcindstrect.MinX = srcrect.MinX + n->leftedge; 
        srcindstrect.MaxX = srcrect.MaxX + n->leftedge;
        srcindstrect.MinY = srcrect.MinY + n->topedge;
        srcindstrect.MaxY = srcrect.MaxY + n->topedge;
        
        /* Find intersection of bitmap visible screen rect and srcindst rect */
        if (AndRectRect(&srcindstrect, &n->screenvisiblerect, &dstandvisrect))
        {
            /* Intersection is valid. Blit. */        

            HIDD_Gfx_CopyBox(
                compdata->gfx,
                bm,
                /* Transform back to source bitmap coord system */
                dstandvisrect.MinX - n->leftedge, dstandvisrect.MinY - n->topedge,
                compdata->compositedbitmap,
                dstandvisrect.MinX, dstandvisrect.MinY,
                dstandvisrect.MaxX - dstandvisrect.MinX + 1,
                dstandvisrect.MaxY - dstandvisrect.MinY + 1,
                compdata->gc);
        }
    }
}

static VOID HIDDCompositingRedrawVisibleScreen(struct HIDDCompositingData * compdata)
{
    struct StackBitMapNode * n = NULL;
    ULONG lastscreenvisibleline = compdata->screenrect.MaxY;
    
    /* Calculations are performed regardless if compositedbitmap is beeing show.
       Gfx operations are only performed if compositedbitmap is beeing show */
    
    /* Recalculate visible rects per screen */
    HIDDCompositingRecalculateVisibleRects(compdata);
    
    /* Refresh all bitmaps on stack */
    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->isscreenvisible)
        {
            IPTR width, height;
            OOP_GetAttr(n->bm, aHidd_BitMap_Width, &width);
            OOP_GetAttr(n->bm, aHidd_BitMap_Height, &height);

            HIDDCompositingRedrawBitmap(compdata, n->bm, 0, 0, width, height);
            if (lastscreenvisibleline > n->screenvisiblerect.MinY)
                lastscreenvisibleline = n->screenvisiblerect.MinY;
        }
    }

    /* Clean up area revealed by drag */
    /* TODO: Find all areas which might have been releaved, not only top - 
       This will happen when there are bitmaps of different sizes composited */
    if (lastscreenvisibleline > 1)
    {
        HIDD_BM_FillRect(compdata->compositedbitmap, 
            compdata->gc, 0, 0, compdata->screenrect.MaxX, lastscreenvisibleline - 1);
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
     * has a negative impact on performance.
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

    D(bug("[Compositing] Toggle te %d, oldcomp 0x%lx, top 0x%lx, comp 0x%lx, newscreen %lx\n",
        topedge, oldcompositedbitmap, compdata->topbitmap, compdata->compositedbitmap, newscreenbitmap));

    /*
     * (e) If the screenbitmap changed, show the new screenbitmap.
     * We do it after refreshing, for better visual appearance.
     */
    if (newscreenbitmap)
    	compdata->screenbitmap = HIDD_Gfx_Show(compdata->gfx, newscreenbitmap, fHidd_Gfx_Show_CopyBack);

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
    BOOL newtop;
    BOOL ok = TRUE;

    D(bug("[Compositing] BitMapStackChanged, topbitmap: 0x%lx\n", msg->data->Bitmap));

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

    /* Switch mode if needed */
    newtop = HIDDCompositingTopBitMapChanged(compdata, msg->data->Bitmap);

    /* Copy bitmaps pointers to our stack */
    for (vpdata = msg->data; vpdata; vpdata = vpdata->Next)
    {
        D(bug("[Compositing] Testing bitmap: 0x%lx\n", vpdata->Bitmap));

        /*
         * Check if the passed bitmap can be composited together with screen
         * bitmap
         */
        if (HIDDCompositingCanCompositeWithScreenBitMap(compdata, vpdata->Bitmap))
        {
            n = AllocMem(sizeof(struct StackBitMapNode), MEMF_ANY | MEMF_CLEAR);
	    if (!n)
	    {
		/*
		 * Error happened.
		 * We need to reset own state and return NULL. graphics.library
		 * falls back to no composition in this case.
		 */
	    	D(bug("[Compositing] Error allocating StackBitMapNode!!!\n"));

		ok = FALSE;
		break;
	    }

	    D(bug("[Compositing] ViewPort 0x%p, offset (%d, %d)\n", vpdata->vpe->ViewPort, vpdata->vpe->ViewPort->DxOffset, vpdata->vpe->ViewPort->DyOffset));

            n->bm               = vpdata->Bitmap;
            n->isscreenvisible  = FALSE;
            n->displayedwidth   = compdata->screenrect.MaxX + 1;
            n->displayedheight  = compdata->screenrect.MaxY + 1;
            n->leftedge		= vpdata->vpe->ViewPort->DxOffset;
            n->topedge		= vpdata->vpe->ViewPort->DyOffset;

            AddTail((struct List *)&compdata->bitmapstack, (struct Node *)n);
        }
    }

    if (ok)
    {
    	/*
    	 * Validate bitmap offsets - they might not match the compositing rules taking
       	 * new displayedwidth/displayedheight values
       	 */
	ForeachNode(&compdata->bitmapstack, n)
    	{
            HIDDCompositingValidateBitMapPositionChange(n->bm, &n->leftedge, &n->topedge, n->displayedwidth, n->displayedheight);
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

    /* Tell if the composition is active */
    *msg->active = compdata->compositedbitmap ? TRUE : FALSE;
    /* Return actually displayed bitmap */
    return compdata->screenbitmap;
}

VOID METHOD(Compositing, Hidd_Compositing, BitMapRectChanged)
{
    struct HIDDCompositingData * compdata = OOP_INST_DATA(cl, o);

    LOCK_COMPOSITING_READ

    HIDDCompositingRedrawBitmap(compdata, msg->bm, msg->x, msg->y, msg->width, msg->height);
    
    UNLOCK_COMPOSITING
}

BOOL METHOD(Compositing, Hidd_Compositing, BitMapPositionChange)
{
    struct HIDDCompositingData *compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode *n;
    IPTR disp_width, disp_height;

    LOCK_COMPOSITING_WRITE

    n = HIDDCompositingIsBitMapOnStack(compdata, msg->bm);
    if (n)
    {
        disp_width  = n->displayedwidth;
        disp_height = n->displayedheight;
    }
    else
    {
	/* The bitmap is not displayed yet. Validate against full size. */
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

    D(bug("[Compositing] Validating bitmap 0x%p, position (%ld, %ld), limits %ld x %ld\n",
    	  msg->bm, *msg->newxoffset, *msg->newyoffset, disp_width, disp_height));

    HIDDCompositingValidateBitMapPositionChange(msg->bm, msg->newxoffset, msg->newyoffset,
    						disp_width, disp_height);

    D(bug("[Compositing] Validated position (%ld, %ld)\n", *msg->newxoffset, *msg->newyoffset));

    if (n && ((*msg->newxoffset != n->leftedge) || (*msg->newyoffset != n->topedge)))
    {
    	D(bug("[Compositing] Old position (%ld, %ld)\n", n->leftedge, n->topedge));

	/* Reflect the change if it happened */
    	n->leftedge = *msg->newxoffset;
    	n->topedge  = *msg->newyoffset;

    	if (compdata->topbitmap == msg->bm)
    	{
    	    /*
    	     * If this is the frontmost bitmap, we may want to toggle compositing,
    	     * if it starts/stops covering the whole screen at one point.
    	     */
            HIDDCompositingToggleCompositing(compdata, FALSE);    
        }
    }

    UNLOCK_COMPOSITING

    /* Return active state */
    return compdata->compositedbitmap ? TRUE : FALSE;
}

static VOID RedrawVisibleRect(struct HIDDCompositingData *compdata, WORD x, WORD y, WORD width, WORD height)
{
    struct StackBitMapNode *n = NULL;
    struct Rectangle tmprect = {x, y, x + width - 1, y + height - 1}, slice;

    /* Draw a slice from each bitmap that intersects the redraw region */
    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->isscreenvisible)
        {
            if (AndRectRect(&n->screenvisiblerect, &tmprect, &slice))
            {
                HIDDCompositingRedrawBitmap(compdata, n->bm,
                    slice.MinX - n->leftedge,
                    slice.MinY - n->topedge,
                    slice.MaxX - slice.MinX + 1,
                    slice.MaxY - slice.MinY + 1);
            }
        }
    }
}

VOID METHOD(Compositing, Hidd_Compositing, DisplayRectChanged)
{
    struct HIDDCompositingData *compdata = OOP_INST_DATA(cl, o);

    LOCK_COMPOSITING_READ

    RedrawVisibleRect(compdata, msg->x, msg->y,
        msg->width, msg->height);

    UNLOCK_COMPOSITING
}

#define NUM_Compositing_Root_METHODS 2

static const struct OOP_MethodDescr Compositing_Root_descr[] =
{
    {(OOP_MethodFunc)Compositing__Root__New, moRoot_New},
    {(OOP_MethodFunc)Compositing__Root__Get, moRoot_Get},
    {NULL, 0}
};

#define NUM_Compositing_Hidd_Compositing_METHODS 4

static const struct OOP_MethodDescr Compositing_Hidd_Compositing_descr[] =
{
    {(OOP_MethodFunc)Compositing__Hidd_Compositing__BitMapStackChanged, moHidd_Compositing_BitMapStackChanged},
    {(OOP_MethodFunc)Compositing__Hidd_Compositing__BitMapRectChanged, moHidd_Compositing_BitMapRectChanged},
    {(OOP_MethodFunc)Compositing__Hidd_Compositing__BitMapPositionChange, moHidd_Compositing_BitMapPositionChange},
    {(OOP_MethodFunc)Compositing__Hidd_Compositing__DisplayRectChanged, moHidd_Compositing_DisplayRectChanged},
    {NULL, 0}
};

const struct OOP_InterfaceDescr Compositing_ifdescr[] =
{
    {Compositing_Root_descr, IID_Root, NUM_Compositing_Root_METHODS},
    {Compositing_Hidd_Compositing_descr, IID_Hidd_Compositing, NUM_Compositing_Hidd_Compositing_METHODS},
    {NULL, NULL}
};

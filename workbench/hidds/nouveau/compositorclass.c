/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

/* 
    This is ment to be (in future) a generic class that will be capable of
    compositor bitmaps on screen to get effects like screen dragging.
    The code is generic where possible, using abstract OOP_Objects instead of
    concrete driver structures. There are places where nouveau specific calls 
    are performed, however there are only few and can be generilized to 
    (private) methods that should be reimplemented by child classes in each of 
    the drivers.
*/

/* Non generic part */
#include "nouveau_intern.h"
/* Non generic part */

#include "compositor_intern.h"

#include <proto/exec.h>
#include <aros/debug.h>
#include <proto/oop.h>
#include <proto/utility.h>


#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#undef HiddGCAttrBase
#undef HiddCompositorAttrBase

#define HiddPixFmtAttrBase      (compdata->pixFmtAttrBase)
#define HiddSyncAttrBase        (compdata->syncAttrBase)
#define HiddBitMapAttrBase      (compdata->bitMapAttrBase)
#define HiddGCAttrBase          (compdata->gcAttrBase)
#define HiddCompositorAttrBase (compdata->compositorAttrBase)

#define MAX(a,b) a > b ? a : b
#define MIN(a,b) a < b ? a : b

static BOOL AndRectRect(struct _Rectangle * rect1, struct _Rectangle * rect2,
    struct _Rectangle * intersect)
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

static VOID HIDDCompositorValidateBitMapPositionChange(struct HIDDCompositorData * compdata, OOP_Object * bm,
    LONG * newxoffset, LONG * newyoffset)
{
    struct StackBitMapNode * n = NULL;

    /* Check if passed bitmap is in stack, ignore if not */
    if ((n = HIDDCompositorIsBitMapOnStack(compdata, bm)) != NULL)
    {
        IPTR width, height;
        LONG limit;
        
        OOP_GetAttr(bm, aHidd_BitMap_Width, &width);
        OOP_GetAttr(bm, aHidd_BitMap_Height, &height);
        
        /* Check x position */
        limit = n->displayedwidth - width;
        if (*(newxoffset) > 0)
            *(newxoffset) = 0;

        if (*(newxoffset) < limit)
            *(newxoffset) = limit;

        /* Check y position */
        limit = n->displayedheight - height;
        if (*(newyoffset) > n->displayedheight - 15) /* Limit for drag */
            *(newyoffset) = n->displayedheight - 15;

        if (*(newyoffset) < limit) /* Limit for scroll */
            *(newyoffset) = limit;
    }
}


static VOID HIDDCompositorRecalculateVisibleRects(struct HIDDCompositorData * compdata)
{
    ULONG lastscreenvisibleline = compdata->screenrect.MaxY;
    struct StackBitMapNode * n = NULL;
    
    /* This function assumes bitmapstack is in correct Z order: 
       from top most to bottom most */
    
    ForeachNode(&compdata->bitmapstack, n)
    {
        /*  Stack bitmap bounding boxes equal screen bounding box taking into
            account topedge */
        IPTR topedge;
        struct _Rectangle tmprect;
        
        OOP_GetAttr(n->bm, aHidd_BitMap_TopEdge, &topedge);
        /* Copy screen rect */
        tmprect = compdata->screenrect;
        /* Set bottom and top values */
        tmprect.MinY = topedge;
        tmprect.MaxY = lastscreenvisibleline;
        /* Intersect both to make sure values are withint screen limit */
        if (AndRectRect(&tmprect, &compdata->screenrect, &n->screenvisiblerect))
        {
            lastscreenvisibleline = n->screenvisiblerect.MinY - 1;
            n->isscreenvisible = TRUE;
        }
        else
            n->isscreenvisible = FALSE;

        D(bug("[Compositor] Bitmap 0x%x, visible %d, (%d, %d) , (%d, %d)\n", 
            n->bm, n->isscreenvisible, 
            n->screenvisiblerect.MinX, n->screenvisiblerect.MinY, 
            n->screenvisiblerect.MaxX, n->screenvisiblerect.MaxY));
    }
}

static BOOL HIDDCompositorTopBitMapChanged(struct HIDDCompositorData * compdata, OOP_Object * bm)
{
    OOP_Object * sync = NULL;
    OOP_Object * pf = NULL;
    IPTR modeid, hdisp, vdisp, e, depth;

    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);

    /* Sanity check */
    if (compdata->gfx != (OOP_Object *)e)
    {
        /* Provided top bitmap is not using the same driver as compositor. Fail. */
        D(bug("[Compositor] GfxHidd different than one used by compositor\n"));
        return FALSE;
    }
    
    /* Read display mode properties */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);
    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[Compositor] Invalid ModeID\n"));
        return FALSE;
    }
    
    /* Invalidate screen bitmap if it is set to "previous" top bitmap, since this
       "previous" top bitmap is already gone at this point */
    if (compdata->screenbitmap == compdata->topbitmap)
        compdata->screenbitmap = NULL;

    /* Set the pointer to top bitmap */
    compdata->topbitmap = bm;

    /* If the mode is already visible do nothing */
    if (modeid == compdata->screenmodeid)
        return TRUE;

    /* The mode is different. Need to prepare information needed for compositor */
    {
        /* Get width and height of mode */
        struct pHidd_Gfx_GetMode __getmodemsg = 
        {
            modeID:     modeid,
            syncPtr:    &sync,
            pixFmtPtr:  &pf,
        }, *getmodemsg = &__getmodemsg;
        struct TagItem gctags[] =
        {
            { aHidd_GC_Foreground, (HIDDT_Pixel)0x99999999 }, 
            { TAG_DONE, TAG_DONE }
        };


        getmodemsg->mID = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_GetMode);
        OOP_DoMethod(compdata->gfx, (OOP_Msg)getmodemsg);

        OOP_GetAttr(sync, aHidd_Sync_HDisp, &hdisp);
        OOP_GetAttr(sync, aHidd_Sync_VDisp, &vdisp);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

        /* Store mode information */ 
        compdata->screenmodeid      = modeid;
        compdata->screenrect.MinX   = 0;
        compdata->screenrect.MinY   = 0;
        compdata->screenrect.MaxX   = hdisp - 1;
        compdata->screenrect.MaxY   = vdisp - 1;
        compdata->modeschanged      = TRUE;
        
        /* Get gray foregound */
        if (depth < 24) gctags[0].ti_Data = (HIDDT_Pixel)0x9492;
        
        OOP_SetAttrs(compdata->gc, gctags);
    }

    return TRUE;
}

static BOOL HIDDCompositorCanCompositeWithScreenBitMap(struct HIDDCompositorData * compdata, OOP_Object * bm)
{
    OOP_Object * screenbm = compdata->topbitmap; /* Tread top bitmap as screen bitmap */
    IPTR screenbmwidth, screenbmheight, screenbmstdpixfmt;
    IPTR bmgfx, bmmodeid, bmwidth, bmheight, bmstdpixfmt;

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
        OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &bmgfx);
        OOP_GetAttr(bm, aHidd_BitMap_ModeID, &bmmodeid);
        OOP_GetAttr(bm, aHidd_BitMap_Width, &bmwidth);
        OOP_GetAttr(bm, aHidd_BitMap_Height, &bmheight);
        OOP_GetAttr(bm, aHidd_BitMap_PixFmt, &pf);
        OOP_GetAttr((OOP_Object*)pf, aHidd_PixFmt_StdPixFmt, &bmstdpixfmt);
    }

    /* If bm uses different instances of gfx hidd than screenbm(=composing), they cannot be composited */
    if (compdata->gfx != (OOP_Object *)bmgfx)
        return FALSE;
    
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

static VOID HIDDCompositorRedrawBitmap(struct HIDDCompositorData * compdata,
    OOP_Object * bm, WORD x, WORD y, WORD width, WORD height)
{
    struct StackBitMapNode * n = NULL;
    
    /* Check if compositedbitmap is being displayed right now */
    if (compdata->screenbitmap != compdata->compositedbitmap)
        return;
    
    /* Check if passed bitmap is in stack, ignore if not */
    if ((n = HIDDCompositorIsBitMapOnStack(compdata, bm)) == NULL)
        return;

    if (!n->isscreenvisible)
        return;

    if (compdata->compositedbitmap)
    {
        IPTR leftedge, topedge;
        struct _Rectangle srcrect;
        struct _Rectangle srcindstrect;
        struct _Rectangle dstandvisrect;

        OOP_GetAttr(bm, aHidd_BitMap_LeftEdge, &leftedge);
        OOP_GetAttr(bm, aHidd_BitMap_TopEdge, &topedge);
        
        /* Rectangle in source bitmap coord system */
        srcrect.MinX = x; 
        srcrect.MinY = y;
        srcrect.MaxX = x + width - 1; 
        srcrect.MaxY = y + height - 1;
        
        /* Source bitmap rectangle in destination (screen) coord system */
        srcindstrect.MinX = srcrect.MinX + leftedge; 
        srcindstrect.MaxX = srcrect.MaxX + leftedge;
        srcindstrect.MinY = srcrect.MinY + topedge;
        srcindstrect.MaxY = srcrect.MaxY + topedge;
        
        /* Find intersection of bitmap visible screen rect and srcindst rect */
        if (AndRectRect(&srcindstrect, &n->screenvisiblerect, &dstandvisrect))
        {
            /* Intersection is valid. Blit. */        

            HIDD_Gfx_CopyBox(
                compdata->gfx,
                bm,
                /* Transform back to source bitmap coord system */
                dstandvisrect.MinX - leftedge, dstandvisrect.MinY - topedge,
                compdata->compositedbitmap,
                dstandvisrect.MinX, dstandvisrect.MinY,
                dstandvisrect.MaxX - dstandvisrect.MinX + 1,
                dstandvisrect.MaxY - dstandvisrect.MinY + 1,
                compdata->gc);
        }
    }
}

static VOID HIDDCompositorRedrawVisibleScreen(struct HIDDCompositorData * compdata)
{
    struct StackBitMapNode * n = NULL;
    ULONG lastscreenvisibleline = compdata->screenrect.MaxY;
    
    /* Calculations are performed regardless if compositedbitmap is beeing show.
       Gfx operations are only performed if compositedbitmap is beeing show */
    
    /* Recalculate visible rects per screen */
    HIDDCompositorRecalculateVisibleRects(compdata);
    
    /* Refresh all bitmaps on stack */
    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->isscreenvisible)
        {
            IPTR width, height;
            OOP_GetAttr(n->bm, aHidd_BitMap_Width, &width);
            OOP_GetAttr(n->bm, aHidd_BitMap_Height, &height);

            HIDDCompositorRedrawBitmap(compdata, n->bm, 0, 0, width, height);
            if (lastscreenvisibleline > n->screenvisiblerect.MinY)
                lastscreenvisibleline = n->screenvisiblerect.MinY;
        }
    }

    /* Clean up area revealed by drag */
    /* TODO: Find all areas which might have been releaved, not only top - 
       This will happen when there are bitmaps of different sizes composited */
    if ((compdata->screenbitmap == compdata->compositedbitmap) && (lastscreenvisibleline > 1))
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
(c) if (ntb->nsf) sb=cb 
(d) if (ntb->fs) sb=ntb

Additional rule:
(e) if (oldsb!=sb) modeswitch(sb)
    
*/
static VOID HIDDCompositorToggleCompositing(struct HIDDCompositorData * compdata)
{
    /* If the topbitmap covers the complete screen, show it instead of 
       compositedbitmap. This removes the need for copying 
       screen bitmap -> composited bitmap. Not copying improves performance */
    IPTR topedge;
    OOP_Object * oldscreenbitmap = compdata->screenbitmap;
    OOP_Object * oldcompositedbitmap = NULL;
    
    OOP_GetAttr(compdata->topbitmap, aHidd_BitMap_TopEdge, &topedge);

    /* (a) */
    if ((compdata->compositedbitmap) && (compdata->modeschanged))
    {
        oldcompositedbitmap = compdata->compositedbitmap;
        compdata->compositedbitmap = NULL;
    }

    
    /* This condition is enough as compositor allows only dragging screen down
       and not up/left/right */
    if ((LONG)topedge > (LONG)0) /* Explicitly cast to get signed comparison */
    {
        /* (b) */
        if (compdata->compositedbitmap == NULL)
        {
            struct TagItem bmtags[5];
            
            /* Create a new bitmap that will be used for compositor */
            bmtags[0].ti_Tag = aHidd_BitMap_Width;          bmtags[0].ti_Data = compdata->screenrect.MaxX + 1;
            bmtags[1].ti_Tag = aHidd_BitMap_Height;         bmtags[1].ti_Data = compdata->screenrect.MaxY + 1;
            bmtags[2].ti_Tag = aHidd_BitMap_Displayable;    bmtags[2].ti_Data = TRUE;
            bmtags[3].ti_Tag = aHidd_BitMap_ModeID;         bmtags[3].ti_Data = compdata->screenmodeid;
            bmtags[4].ti_Tag = TAG_DONE;                    bmtags[4].ti_Data = TAG_DONE;

            compdata->compositedbitmap = HIDD_Gfx_NewBitMap(compdata->gfx, bmtags);
        }
        
        /* (c) */
        if (oldscreenbitmap != compdata->compositedbitmap)
        {
            compdata->screenbitmap = compdata->compositedbitmap;
        
            /* Redraw bitmap stack - compensate for changes that happened while
               compositor was not active */
            HIDDCompositorRedrawVisibleScreen(compdata);
        }
    }
    else
    {
        /* (d) */
        compdata->screenbitmap = compdata->topbitmap;
    }

    D(bug("[Compositor] Toggle te %d, oldscr 0x%x, top 0x%x, comp 0x%x, scr 0x%x\n",
        topedge, oldscreenbitmap, compdata->topbitmap, compdata->compositedbitmap, 
        compdata->screenbitmap));

    /* If the screenbitmap changed, show the new screenbitmap */
    /* (e) */
    if (oldscreenbitmap != compdata->screenbitmap)
        HIDDNouveauSwitchToVideoMode(compdata->screenbitmap);

    /* (a) - disposing of oldcompositorbitmap needs to happen after mode switch 
       since it could have been the current screenbitmap */
    if (oldcompositedbitmap)
        HIDD_Gfx_DisposeBitMap(compdata->gfx, oldcompositedbitmap);

    /* Handled */
    compdata->modeschanged = FALSE;
}

static VOID HIDDCompositorPurgeBitMapStack(struct HIDDCompositorData * compdata)
{
    struct StackBitMapNode * curr, * next;

    ForeachNodeSafe(&compdata->bitmapstack, curr, next)
    {
        Remove((struct Node *)curr);
        FreeMem(curr, sizeof(struct StackBitMapNode));
    }
    
    NEWLIST(&compdata->bitmapstack);
}




/* PUBLIC METHODS */
OOP_Object *METHOD(Compositor, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if(o)
    {
        struct HIDDCompositorData * compdata = OOP_INST_DATA(cl, o);
        
        struct OOP_ABDescr attrbases[] = 
        {
        { IID_Hidd_PixFmt,          &compdata->pixFmtAttrBase },
        { IID_Hidd_Sync,            &compdata->syncAttrBase },
        { IID_Hidd_BitMap,          &compdata->bitMapAttrBase },
        { IID_Hidd_GC,              &compdata->gcAttrBase },
        { IID_Hidd_Compositor,     &compdata->compositorAttrBase },
        { NULL, NULL }
        };

        NEWLIST(&compdata->bitmapstack);
        compdata->compositedbitmap  = NULL;
        compdata->topbitmap         = NULL;
        compdata->screenbitmap      = NULL;
        compdata->screenmodeid      = vHidd_ModeID_Invalid;
        compdata->modeschanged      = FALSE;
        InitSemaphore(&compdata->semaphore);
        
        /* Obtain Attr bases - make this class self-contained */
        if (OOP_ObtainAttrBases(attrbases))
        {
            compdata->gfx = (OOP_Object *)GetTagData(aHidd_Compositor_GfxHidd, 0, msg->attrList);
            
            if (compdata->gfx != NULL)
            {
                /* Create GC object that will be used for drawing operations */
                compdata->gc = HIDD_Gfx_NewGC(compdata->gfx, NULL);
            }
        }
        
        if ((compdata->gfx == NULL) || (compdata->gc == NULL))
        {
            /* Creation failed */
            OOP_MethodID disposemid;
            disposemid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg)&disposemid);
            o = NULL;
        }
    }

    return o;
}

VOID METHOD(Compositor, Hidd_Compositor, BitMapStackChanged)
{
    struct HIDD_ViewPortData * vpdata;
    struct HIDDCompositorData * compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode * n = NULL;

    D(bug("[Compositor] BitMapStackChanged, topbitmap: 0x%x\n", 
        msg->data->Bitmap));

    LOCK_COMPOSITOR_WRITE
        
    /* Free all items which are already on the list */
    HIDDCompositorPurgeBitMapStack(compdata);
    
    
    if (!msg->data)
    {
        UNLOCK_COMPOSITOR
        return; /* TODO: BLANK SCREEN */
    }
    
    /* Switch mode if needed */    
    if (!HIDDCompositorTopBitMapChanged(compdata, msg->data->Bitmap))
    {
        /* Something bad happened. Yes, bitmap stack is already erased - that's ok */
        D(bug("[Compositor] Failed to change top bitmap\n"));
        UNLOCK_COMPOSITOR
        return; 
    }
    
    /* Copy bitmaps pointers to our stack */
    for (vpdata = msg->data; vpdata; vpdata = vpdata->Next)
    {
        D(bug("Compositor] Testing bitmap: %x\n", vpdata->Bitmap));
        /* Check if the passed bitmap can be composited together with screen
           bitmap */
        if (HIDDCompositorCanCompositeWithScreenBitMap(compdata, vpdata->Bitmap))
        {
            n = AllocMem(sizeof(struct StackBitMapNode), MEMF_ANY | MEMF_CLEAR);

            n->bm               = vpdata->Bitmap;
            n->isscreenvisible  = FALSE;
            n->displayedwidth   = compdata->screenrect.MaxX + 1;
            n->displayedheight  = compdata->screenrect.MaxY + 1;
            AddTail(&compdata->bitmapstack, (struct Node *)n);
        }
    }
    
    /* Validate bitmap offsets - they might not match the compositor rules taking
       new displayedwidth/displayedheight values */
    ForeachNode(&compdata->bitmapstack, n)
    {
        LONG newxoffset, newyoffset;
        IPTR val;
        OOP_GetAttr(n->bm, aHidd_BitMap_TopEdge, &val);newyoffset = (LONG)val;
        OOP_GetAttr(n->bm, aHidd_BitMap_LeftEdge, &val);newxoffset = (LONG)val;
        
        HIDDCompositorValidateBitMapPositionChange(compdata, n->bm, 
            &newxoffset, &newyoffset);

        /* Override offsets without checks present in bitmap Set method */
        HIDDNouveauSetOffsets(n->bm, newxoffset, newyoffset);
    }

    /* Toogle compositor based on screen positions */
    HIDDCompositorToggleCompositing(compdata);

    /* Redraw bitmap stack - compensate for change of visible rects 
       resulting from new top bitmap */
    HIDDCompositorRedrawVisibleScreen(compdata);

    UNLOCK_COMPOSITOR
}

VOID METHOD(Compositor, Hidd_Compositor, BitMapRectChanged)
{
    struct HIDDCompositorData * compdata = OOP_INST_DATA(cl, o);

    LOCK_COMPOSITOR_READ

    HIDDCompositorRedrawBitmap(compdata, msg->bm, msg->x, msg->y, msg->width, msg->height);
    
    UNLOCK_COMPOSITOR
}

VOID METHOD(Compositor, Hidd_Compositor, BitMapPositionChanged)
{
    struct HIDDCompositorData * compdata = OOP_INST_DATA(cl, o);

    LOCK_COMPOSITOR_WRITE

    /* Check is passed bitmap is in stack, ignore if not */
    if (HIDDCompositorIsBitMapOnStack(compdata, msg->bm) != NULL)
    {
        /* If top bitmap position has changed, possibly toggle compositor */
        if (compdata->topbitmap == msg->bm)
            HIDDCompositorToggleCompositing(compdata);

        /* If compositor is not active and top bitmap has changed, execute scroll */
        if ((compdata->screenbitmap == compdata->topbitmap)
            && (compdata->topbitmap == msg->bm))
        {
            
            HIDDNouveauSwitchToVideoMode(compdata->screenbitmap);
        }
        else
        {
            /* Redraw bitmap stack - compensate change of visible rects resulting
               from move of bitmap */
            HIDDCompositorRedrawVisibleScreen(compdata);
        }
    }
    
    UNLOCK_COMPOSITOR
}

VOID METHOD(Compositor, Hidd_Compositor, ValidateBitMapPositionChange)
{
    struct HIDDCompositorData * compdata = OOP_INST_DATA(cl, o);

    LOCK_COMPOSITOR_READ
    
    HIDDCompositorValidateBitMapPositionChange(compdata, msg->bm, 
        msg->newxoffset, msg->newyoffset);
    
    UNLOCK_COMPOSITOR
}


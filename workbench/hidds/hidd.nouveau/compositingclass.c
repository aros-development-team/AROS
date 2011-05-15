/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/* 
    This is ment to be (in future) a generic class that will be capable of
    compositing bitmaps on screen to get effects like screen dragging.
    The code is generic where possible, using abstract OOP_Objects instead of
    concrete driver structures. There are places where nouveau specific calls 
    are performed, however there are only few and can be generilized to 
    (private) methods that should be reimplemented by child classes in each of 
    the drivers.
*/

/* Non generic part */
#include "nouveau_intern.h"
/* Non generic part */

#include "compositing_intern.h"

#include <proto/exec.h>
#include <aros/debug.h>
#include <proto/oop.h>
#include <proto/utility.h>

#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#undef HiddGCAttrBase
#undef HiddCompositingAttrBase

#define HiddSyncAttrBase        (SD(cl)->syncAttrBase)
#define HiddBitMapAttrBase      (SD(cl)->bitMapAttrBase)
#define HiddPixFmtAttrBase      (SD(cl)->pixFmtAttrBase)
#define HiddGCAttrBase          (SD(cl)->gcAttrBase)
#define HiddCompositingAttrBase (SD(cl)->compositingAttrBase)

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
        IPTR topedge;
        struct _Rectangle tmprect;
        OOP_Class * cl = OOP_OCLASS(n->bm);
        
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

        D(bug("[Compositing] Bitmap 0x%x, visible %d, (%d, %d) , (%d, %d)\n", 
            n->bm, n->isscreenvisible, 
            n->screenvisiblerect.MinX, n->screenvisiblerect.MinY, 
            n->screenvisiblerect.MaxX, n->screenvisiblerect.MaxY));
    }
}

static VOID HIDDCompositingRecalculateDisplayedWidthHeight(struct HIDDCompositingData * compdata)
{
    struct StackBitMapNode * n = NULL;

    ForeachNode(&compdata->bitmapstack, n)
    {
        n->displayedwidth   = compdata->screenrect.MaxX + 1;
        n->displayedheight  = compdata->screenrect.MaxY + 1;
    }
}

static BOOL HIDDCompositingTopBitMapChanged(struct HIDDCompositingData * compdata, OOP_Object * bm)
{
    /* 
        Desctiption: 
        a) take incomming top bitmap
        b) read its mode and sizes,
        c) create a mirroring bitmap that fits the mode
        d) switch mode (driver dependandant)
    */

    OOP_Class * cl = OOP_OCLASS(bm);
    OOP_Object * sync = NULL;
    OOP_Object * pf = NULL;
    OOP_Object * fbbitmap = NULL;
    IPTR modeid, hdisp, vdisp, e, depth;
    struct TagItem bmtags[5];

    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);

    /* Sanity check */
    if (compdata->gfx != (OOP_Object *)e)
    {
        /* Provided top bitmap is not using the same driver as compositing. Fail. */
        D(bug("[Compositing] GfxHidd different than one used by compositing\n"));
        return FALSE;
    }
    
    /* Read display mode properties */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);
    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[Compositing] Invalid ModeID\n"));
        return FALSE;
    }

    /* Set the pointer to top and screen bitmap */
    compdata->topbitmap     = bm;

    /* If the mode is already visible do nothing */
    if (modeid == compdata->screenmodeid)
        return TRUE;
    
    /* Get width and height of mode */
    struct pHidd_Gfx_GetMode __getmodemsg = 
    {
        modeID:	modeid,
        syncPtr:	&sync,
        pixFmtPtr:	&pf,
    }, *getmodemsg = &__getmodemsg;

    getmodemsg->mID = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_GetMode);
    OOP_DoMethod(compdata->gfx, (OOP_Msg)getmodemsg);

    OOP_GetAttr(sync, aHidd_Sync_HDisp, &hdisp);
    OOP_GetAttr(sync, aHidd_Sync_VDisp, &vdisp);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

    /* Create a new bitmap that will be used for framebuffer */
    bmtags[0].ti_Tag = aHidd_BitMap_Width;          bmtags[0].ti_Data = hdisp;
    bmtags[1].ti_Tag = aHidd_BitMap_Height;         bmtags[1].ti_Data = vdisp;
    bmtags[2].ti_Tag = aHidd_BitMap_Displayable;    bmtags[2].ti_Data = TRUE;
    bmtags[3].ti_Tag = aHidd_BitMap_ModeID;         bmtags[3].ti_Data = modeid;
    bmtags[4].ti_Tag = TAG_DONE;                    bmtags[4].ti_Data = TAG_DONE;
    
    fbbitmap = HIDD_Gfx_NewBitMap(compdata->gfx, bmtags);
    if (fbbitmap)
    {
        BOOL ret = HIDDNouveauSwitchToVideoMode(fbbitmap);
        if (ret)
        {
            struct TagItem gctags[] =
            {
                { aHidd_GC_Foreground, (HIDDT_Pixel)0x99999999 }, 
                { TAG_DONE, TAG_DONE }
            };


            /* Dispose the previous screenbitmap */
            if (compdata->compositedbitmap)
                HIDD_Gfx_DisposeBitMap(compdata->gfx, compdata->compositedbitmap);

            /* Store bitmap/mode information */ 
            compdata->screenmodeid      = modeid;
            compdata->compositedbitmap  = fbbitmap;
            compdata->screenrect.MinX   = 0;
            compdata->screenrect.MinY   = 0;
            compdata->screenrect.MaxX   = hdisp - 1;
            compdata->screenrect.MaxY   = vdisp - 1;
            
            /* Get gray foregound */
            if (depth < 24) gctags[0].ti_Data = (HIDDT_Pixel)0x9492;
            OOP_SetAttrs(compdata->gc, gctags);
            
            return TRUE;
        }
        else
        {
            /* Dispose fbbitmap */
            HIDD_Gfx_DisposeBitMap(compdata->gfx, fbbitmap);
            return FALSE;
        }
    }

    return FALSE;
}

static BOOL HIDDCompositingCanCompositeWithScreenBitMap(struct HIDDCompositingData * compdata, OOP_Object * bm)
{
    /* FIXME: what to do here when it will be possible to have NULL compositedbitmat */
    OOP_Object * screenbm = compdata->compositedbitmap;
    IPTR screenbmwidth, screenbmheight, screenbmstdpixfmt;
    IPTR bmgfx, bmmodeid, bmwidth, bmheight, bmstdpixfmt;

    
    /* HINT: both bitmaps can have different classes */
    {
        OOP_Class * cl = OOP_OCLASS(screenbm);
        IPTR pf;
        OOP_GetAttr(screenbm, aHidd_BitMap_Width, &screenbmwidth);
        OOP_GetAttr(screenbm, aHidd_BitMap_Height, &screenbmheight);
        OOP_GetAttr(screenbm, aHidd_BitMap_PixFmt, &pf);
        OOP_GetAttr((OOP_Object*)pf, aHidd_PixFmt_StdPixFmt, &screenbmstdpixfmt);
    }

    {
        OOP_Class * cl = OOP_OCLASS(bm);
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

static VOID HIDDCompositingRedrawBitmap(struct HIDDCompositingData * compdata,
    OOP_Object * bm, WORD x, WORD y, WORD width, WORD height)
{
    struct StackBitMapNode * n = NULL;
    
    /* Check if compositedbitmap is beeing displayed right now */
    if (compdata->screenbitmap != compdata->compositedbitmap)
        return;
    
    /* Check if passed bitmap is in stack, ignore if not */
    if ((n = HIDDCompositingIsBitMapOnStack(compdata, bm)) == NULL)
        return;

    if (!n->isscreenvisible)
        return;

    if (compdata->compositedbitmap)
    {
        OOP_Class * cl = OOP_OCLASS(bm);
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

static VOID HIDDCompositingRedrawVisibleScreen(struct HIDDCompositingData * compdata)
{
    struct StackBitMapNode * n = NULL;
    ULONG lastscreenvisibleline = compdata->screenrect.MaxY;
    
    /* Recalculate visible rects per screen */
    HIDDCompositingRecalculateVisibleRects(compdata);
    
    /* Refresh all bitmaps on stack */
    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->isscreenvisible)
        {
            OOP_Class * cl = OOP_OCLASS(n->bm);
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

static VOID HIDDCompositingToggleCompositing(struct HIDDCompositingData * compdata)
{
    /* If the topbitmap covers the complete screen, show it instead of 
       compositedbitmap. This removes the need for copying 
       screen bitmap -> composited bitmap. Not copying improves performance */
    IPTR topedge;
    OOP_Class * cl = OOP_OCLASS(compdata->topbitmap);
    OOP_Object * oldscreenbitmap = compdata->screenbitmap;
    
    OOP_GetAttr(compdata->topbitmap, aHidd_BitMap_TopEdge, &topedge);
    
    /* This condition is enought as compositing allows only dragging screen down
       and not up/left/right */
    if (topedge > 0)
    {
        if (oldscreenbitmap != compdata->compositedbitmap)
        {
            compdata->screenbitmap = compdata->compositedbitmap;
        
            /* Redraw bitmap stack - compensate for changes that happened while
               compositing was not active */
            HIDDCompositingRedrawVisibleScreen(compdata);
        }
    }
    else
        compdata->screenbitmap = compdata->topbitmap;

    D(bug("[Compositing] Toggle oldscr 0x%x, top 0x%x, comp 0x%x, scr 0x%x\n", 
        oldscreenbitmap, compdata->topbitmap, compdata->compositedbitmap, 
        compdata->screenbitmap));

    /* If the screenbitmap changed, show the new screenbitmap */
    if (oldscreenbitmap != compdata->screenbitmap)
        HIDDNouveauSwitchToVideoMode(compdata->screenbitmap);
}

static VOID HIDDCompositingPurgeBitMapStack(struct HIDDCompositingData * compdata)
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
OOP_Object *METHOD(Compositing, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if(o)
    {
        struct HIDDCompositingData * compdata = OOP_INST_DATA(cl, o);

        NEWLIST(&compdata->bitmapstack);
        compdata->compositedbitmap  = NULL;
        compdata->topbitmap         = NULL;
        compdata->screenbitmap      = NULL;
        compdata->screenmodeid      = vHidd_ModeID_Invalid;
        InitSemaphore(&compdata->semaphore);
        
        compdata->gfx = (OOP_Object *)GetTagData(aHidd_Compositing_GfxHidd, 0, msg->attrList);
        
        if (compdata->gfx != NULL)
        {
            /* Create GC object that will be used for drawing operations */
            compdata->gc = HIDD_Gfx_NewGC(compdata->gfx, NULL);
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

VOID METHOD(Compositing, Hidd_Compositing, BitMapStackChanged)
{
    struct HIDD_ViewPortData * vpdata;
    struct HIDDCompositingData * compdata = OOP_INST_DATA(cl, o);

    D(bug("[Compositing] BitMapStackChanged, topbitmap: 0x%x\n", 
        msg->data->Bitmap));

    LOCK_COMPOSITING_WRITE
        
    /* Free all items which are already on the list */
    HIDDCompositingPurgeBitMapStack(compdata);
    
    
    if (!msg->data)
    {
        UNLOCK_COMPOSITING
        return; /* TODO: BLANK SCREEN */
    }
    
    /* Switch mode if needed */    
    if (!HIDDCompositingTopBitMapChanged(compdata, msg->data->Bitmap))
    {
        /* Something bad happened. Yes, bitmap stack is already erased - that's ok */
        D(bug("[Compositing] Failed to change top bitmap\n"));
        UNLOCK_COMPOSITING
        return; 
    }
    
    /* Copy bitmaps pointers to our stack */
    for (vpdata = msg->data; vpdata; vpdata = vpdata->Next)
    {
        D(bug("Compositing] Testing bitmap: %x\n", vpdata->Bitmap));
        /* Check if the passed bitmap can be composited together with screen
           bitmap */
        if (HIDDCompositingCanCompositeWithScreenBitMap(compdata, vpdata->Bitmap))
        {
            struct StackBitMapNode * n = AllocMem(sizeof(struct StackBitMapNode), MEMF_ANY | MEMF_CLEAR);
            n->bm               = vpdata->Bitmap;
            n->isscreenvisible  = FALSE;
            n->displayedwidth   = 0;
            n->displayedheight  = 0;
            AddTail(&compdata->bitmapstack, (struct Node *)n);
        }
    }

    /* Set displayedwidth/displayedheight on all screen bitmaps */
    HIDDCompositingRecalculateDisplayedWidthHeight(compdata);
    
    /* Toogle compositing based on screen positions */
    HIDDCompositingToggleCompositing(compdata);

    /* Redraw bitmap stack - compensate for change of visible rects 
       resulting from new top bitmap */
    HIDDCompositingRedrawVisibleScreen(compdata);

    UNLOCK_COMPOSITING
}

VOID METHOD(Compositing, Hidd_Compositing, BitMapRectChanged)
{
    struct HIDDCompositingData * compdata = OOP_INST_DATA(cl, o);

    LOCK_COMPOSITING_READ

    HIDDCompositingRedrawBitmap(compdata, msg->bm, msg->x, msg->y, msg->width, msg->height);
    
    UNLOCK_COMPOSITING
}

VOID METHOD(Compositing, Hidd_Compositing, BitMapPositionChanged)
{
    struct HIDDCompositingData * compdata = OOP_INST_DATA(cl, o);

    LOCK_COMPOSITING_WRITE

    /* Check is passed bitmap is in stack, ignore if not */
    if (HIDDCompositingIsBitMapOnStack(compdata, msg->bm) != NULL)
    {
        /* If top bitmap position has changed, possibly toggle compositing */
        if (compdata->topbitmap == msg->bm)
            HIDDCompositingToggleCompositing(compdata);

        /* Redraw bitmap stack - compensate change of visible rects resulting
           from move of bitmap */
        HIDDCompositingRedrawVisibleScreen(compdata);
    }
    
    UNLOCK_COMPOSITING
}

VOID METHOD(Compositing, Hidd_Compositing, ValidateBitMapPositionChange)
{
    struct HIDDCompositingData * compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode * n = NULL;

    LOCK_COMPOSITING_READ
    
    /* Check if passed bitmap is in stack, ignore if not */
    if ((n = HIDDCompositingIsBitMapOnStack(compdata, msg->bm)) != NULL)
    {
        IPTR width, height;
        LONG limit;
        
        OOP_GetAttr(msg->bm, aHidd_BitMap_Width, &width);
        OOP_GetAttr(msg->bm, aHidd_BitMap_Height, &height);
        
        /* Check x position */
        limit = n->displayedwidth - width;
        if (*(msg->newxoffset) > 0)
            *(msg->newxoffset) = 0;

        if (*(msg->newxoffset) < limit)
            *(msg->newxoffset) = limit;

        /* Check y position */
        limit = n->displayedheight - height;
        if (*(msg->newyoffset) > n->displayedheight - 15) /* Limit for drag */
            *(msg->newyoffset) = n->displayedheight - 15;

        if (*(msg->newyoffset) < limit) /* Limit for scroll */
            *(msg->newyoffset) = limit;
    }
    
    UNLOCK_COMPOSITING
}


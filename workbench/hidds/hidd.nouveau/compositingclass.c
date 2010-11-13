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

#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#undef HiddGCAttrBase //FIXME remove
#define HiddSyncAttrBase    (SD(cl)->syncAttrBase)
#define HiddBitMapAttrBase  (SD(cl)->bitMapAttrBase)
#define HiddPixFmtAttrBase  (SD(cl)->pixFmtAttrBase)
#define HiddGCAttrBase      (SD(cl)->gcAttrBase) //FIXME remove

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
    
    /* TODO: probably add composing wide read lock or lock at upper level? */

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
    
    /* TODO: probably add composing wide read lock or lock at upper level? */
    
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
            lastscreenvisibleline = n->screenvisiblerect.MinY;
            n->isscreenvisible = TRUE;
        }
        else
            n->isscreenvisible = FALSE;

        D(bug("Bitmap %x, visible %d, (%d, %d) , (%d, %d)\n", n->bm, n->isscreenvisible, 
            n->screenvisiblerect.MinX, n->screenvisiblerect.MinY, 
            n->screenvisiblerect.MaxX, n->screenvisiblerect.MaxY));
    }
}

static VOID HIDDCompositingRecalculateDisplayedWidthHeight(struct HIDDCompositingData * compdata)
{
    OOP_Class * cl = OOP_OCLASS(compdata->screenbitmap);
    struct StackBitMapNode * n = NULL;
    IPTR displayedwidth, displayedheight;

    /* TODO: probably add composing wide read lock or lock at upper level? */
    
    OOP_GetAttr(compdata->screenbitmap, aHidd_BitMap_Width, &displayedwidth);
    OOP_GetAttr(compdata->screenbitmap, aHidd_BitMap_Height, &displayedheight);
    
    ForeachNode(&compdata->bitmapstack, n)
    {
        struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(OOP_OCLASS(n->bm), n->bm);
        bmdata->displayedwidth  = displayedwidth;
        bmdata->displayedheight = displayedheight;
    }
}

static BOOL HIDDCompositingTopBitMapChanged(struct HIDDCompositingData * compdata, OOP_Object * bm)
{
    /* 
        Desctiption: 
        take incomming top bitmap
        read its mode and sizes,
        create a mirroring bitmap that fits the mode
        switch mode 
    */
    /* TODO: needs locking mechanism or maybe lock at upper level?*/
    OOP_Class * cl = OOP_OCLASS(bm);
    OOP_Object * gfx = NULL;
    OOP_Object * sync = NULL;
    OOP_Object * pf = NULL;
    OOP_Object * fbbitmap = NULL;
    IPTR modeid, hdisp, vdisp, e;
    struct TagItem bmtags[5];

    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);
    gfx = (OOP_Object *)e;

    /* Read display mode properties */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);
    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[Nouveau] Invalid ModeID\n"));
        return FALSE;
    }

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
    OOP_DoMethod(gfx, (OOP_Msg)getmodemsg);

    OOP_GetAttr(sync, aHidd_Sync_HDisp, &hdisp);
    OOP_GetAttr(sync, aHidd_Sync_VDisp, &vdisp);

    /* Create a new bitmap that will be used for framebuffer */
    bmtags[0].ti_Tag = aHidd_BitMap_Width;          bmtags[0].ti_Data = hdisp;
    bmtags[1].ti_Tag = aHidd_BitMap_Height;         bmtags[1].ti_Data = vdisp;
    bmtags[2].ti_Tag = aHidd_BitMap_Displayable;    bmtags[2].ti_Data = TRUE;
    bmtags[3].ti_Tag = aHidd_BitMap_ModeID;         bmtags[3].ti_Data = modeid;
    bmtags[4].ti_Tag = TAG_DONE;                    bmtags[4].ti_Data = TAG_DONE;
    
    fbbitmap = HIDD_Gfx_NewBitMap(gfx, bmtags);
    if (fbbitmap)
    {
        BOOL ret = HIDDNouveauSwitchToVideoMode(fbbitmap);
        if (ret)
        {
            /* Dispose the previous screenbitmap */
            if (compdata->screenbitmap)
                HIDD_Gfx_DisposeBitMap(gfx, compdata->screenbitmap);

            /* Store bitmap/mode information */ 
            compdata->screenmodeid    = modeid;
            compdata->screenbitmap    = fbbitmap;
            compdata->screenrect.MinX = 0;
            compdata->screenrect.MinY = 0;
            compdata->screenrect.MaxX = hdisp - 1;
            compdata->screenrect.MaxY = vdisp - 1;
            
            return TRUE;
        }
        else
        {
            /* Dispose fbbitmap */
            HIDD_Gfx_DisposeBitMap(gfx, fbbitmap);
            return FALSE;
        }
    }

    return FALSE;
}

static BOOL HIDDCompositingCanCompositeWithScreenBitMap(struct HIDDCompositingData * compdata, OOP_Object * bm)
{
    OOP_Object * screenbm = compdata->screenbitmap;
    IPTR screenbmgfx, screenbmmodeid, screenbmwidth, screenbmheight, screenbmstdpixfmt;
    IPTR bmgfx, bmmodeid, bmwidth, bmheight, bmstdpixfmt;

    
    /* HINT: both bitmaps can have different classes */
    {
        OOP_Class * cl = OOP_OCLASS(screenbm);
        IPTR pf;
        OOP_GetAttr(screenbm, aHidd_BitMap_GfxHidd, &screenbmgfx);
        OOP_GetAttr(screenbm, aHidd_BitMap_ModeID, &screenbmmodeid);
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

    /* If bitmaps use different instances of gfx hidd, they cannot be composited */
    if (screenbmgfx != bmgfx)
        return FALSE;
    
    /* If bitmaps have the same modeid, they can be composited */
    if (screenbmmodeid == bmmodeid)
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
    
    /* Check if passed bitmap is in stack, ignore if not */
    if ((n = HIDDCompositingIsBitMapOnStack(compdata, bm)) == NULL)
        return;

    if (!n->isscreenvisible)
        return;

    if (compdata->screenbitmap)
    {
        OOP_Class * cl = OOP_OCLASS(bm);
        OOP_Object * gfx = NULL; //FIXME remove
        OOP_Object * tmpgc = NULL;
        IPTR e, leftedge, topedge;
        struct _Rectangle srcrect;
        struct _Rectangle srcindstrect;
        struct _Rectangle dstandvisrect;

        OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);
        gfx = (OOP_Object *)e;
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
            /* TODO: stubs have performance hit, change to direct DoMethod call with cached MethodID */

            /* TODO: Cache this option in gfxdata */
            tmpgc = HIDD_Gfx_NewGC(gfx, NULL);        
            HIDD_Gfx_CopyBox(gfx,
                bm,
                /* Transform back to source bitmap coord system */
                dstandvisrect.MinX - leftedge, dstandvisrect.MinY - topedge,
                compdata->screenbitmap,
                dstandvisrect.MinX, dstandvisrect.MinY,
                dstandvisrect.MaxX - dstandvisrect.MinX + 1,
                dstandvisrect.MaxY - dstandvisrect.MinY + 1,
                tmpgc);
            HIDD_Gfx_DisposeGC(gfx, tmpgc);
        }
    }
}

static VOID HIDDCompositingRedrawVisibleScreen(struct HIDDCompositingData * compdata)
{
    /* TODO:probabaly needs a compositing wide read lock or lock at upper level?*/
    struct StackBitMapNode * n = NULL;
    ULONG lastscreenvisibleline = compdata->screenrect.MaxY;
    
    /* Recalculate visible rects per screen */
    HIDDCompositingRecalculateVisibleRects(compdata);
    
    /* TODO: Optimization - if top bitmap covers the complete physical screen,
       the driver can turn off mirroring mode and use the top bitmap as
       framebuffer */

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
    if (lastscreenvisibleline > 0)
    {
        OOP_Class * cl = OOP_OCLASS(compdata->screenbitmap);
        IPTR e, viswidth;
        OOP_Object * gfx = NULL; // FIXME: remove
        OOP_Object * tmpgc = NULL;
        struct TagItem gctags[] =
        {
            { aHidd_GC_Foreground, (HIDDT_Pixel)0x99999999 }, /* TODO: value depends on depth */
            { TAG_DONE, TAG_DONE }
        };
        
        OOP_GetAttr(compdata->screenbitmap, aHidd_BitMap_GfxHidd, &e);
        gfx = (OOP_Object *)e;
        OOP_GetAttr(compdata->screenbitmap, aHidd_BitMap_Width, &viswidth); 

        /* TODO: Cache this option in gfxdata */
        tmpgc = HIDD_Gfx_NewGC(gfx, gctags);
        HIDD_BM_FillRect(compdata->screenbitmap, tmpgc, 0, 0, viswidth, lastscreenvisibleline);
        HIDD_Gfx_DisposeGC(gfx, tmpgc);
    }
}





/* PUBLIC METHODS */
OOP_Object *METHOD(Compositing, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if(o)
    {
        struct HIDDCompositingData * compdata = OOP_INST_DATA(cl, o);

        NEWLIST(&compdata->bitmapstack);
        compdata->screenbitmap  = NULL;
        compdata->screenmodeid  = vHidd_ModeID_Invalid;
    }

    return o;
}

VOID METHOD(Compositing, Hidd_Compositing, BitMapStackChanged)
{
    struct HIDD_ViewPortData * vpdata;
    struct HIDDCompositingData * compdata = OOP_INST_DATA(cl, o);
    /* TODO: probably needs driver wide lock */
    /* TODO: free all items which are already on the list */
    NEWLIST(&compdata->bitmapstack); /* YES THIS IS MEMORY LEAK */
    
    
    if (!msg->data)
        return; /* TODO: BLANK SCREEN */

    /* Switch mode if needed */    
    HIDDCompositingTopBitMapChanged(compdata, msg->data->Bitmap);
    
    /* Copy bitmaps pointers to our stack */
    for (vpdata = msg->data; vpdata; vpdata = vpdata->Next)
    {
        /* Check if the passed bitmap can be composited together with screen
           bitmap */
        if (HIDDCompositingCanCompositeWithScreenBitMap(compdata, vpdata->Bitmap))
        {
            struct StackBitMapNode * n = AllocVec(sizeof(struct StackBitMapNode), MEMF_ANY | MEMF_CLEAR);
            n->bm = vpdata->Bitmap;
            n->isscreenvisible = FALSE;
            AddTail(&compdata->bitmapstack, (struct Node *)n);
        }
    }

    /* Set displayedwidth/displayedheight on all screen bitmaps */
    HIDDCompositingRecalculateDisplayedWidthHeight(compdata);
    
    /* Redraw bitmap stack */
    HIDDCompositingRedrawVisibleScreen(compdata);
}

VOID METHOD(Compositing, Hidd_Compositing, BitMapRectChanged)
{
    /* TODO:probabaly needs a compositing wide read lock */
    struct HIDDCompositingData * compdata = OOP_INST_DATA(cl, o);

    HIDDCompositingRedrawBitmap(compdata, msg->bm, msg->x, msg->y, msg->width, msg->height);
}

VOID METHOD(Compositing, Hidd_Compositing, BitMapPositionChanged)
{
    struct HIDDCompositingData * compdata = OOP_INST_DATA(cl, o);
    /* TODO: probably needs driver wide lock */

    /* Check is passed bitmap is in stack, ignore if not */
    if (HIDDCompositingIsBitMapOnStack(compdata, msg->bm) == NULL)
        return; /* Ignore */
        
    /* Redraw bitmap stack */
    HIDDCompositingRedrawVisibleScreen(compdata);
}



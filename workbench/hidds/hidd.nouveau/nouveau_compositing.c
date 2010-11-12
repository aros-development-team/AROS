/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"
#include "nouveau_compositing.h"

#include "arosdrmmode.h"
#include <aros/debug.h>

#undef HiddPixFmtAttrBase
#undef HiddGfxAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#undef HiddGCAttrBase
#define HiddPixFmtAttrBase  (SD(cl)->pixFmtAttrBase)
#define HiddGfxAttrBase     (SD(cl)->gfxAttrBase)
#define HiddSyncAttrBase    (SD(cl)->syncAttrBase)
#define HiddBitMapAttrBase  (SD(cl)->bitMapAttrBase)
#define HiddGCAttrBase  (SD(cl)->gcAttrBase)

/* This function assumes that the mode, crtc and output are already selected */
static BOOL HIDDNouveauShowBitmapForSelectedMode(OOP_Object * bm)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauData * gfxdata = NULL;
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device_priv *nvdev = nouveau_device(carddata->dev);
    uint32_t output_ids[] = {0};
    uint32_t output_count = 1;
    IPTR e = (IPTR)NULL;
    OOP_Object * gfx = NULL;
    LONG ret;

    LOCK_BITMAP
    
    /* Check if passed bitmap has been registered as framebuffer */
    if (bmdata->fbid == 0)
    {
        UNLOCK_BITMAP
        return FALSE;
    }
    
    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);
    gfx = (OOP_Object *)e;
    gfxdata = OOP_INST_DATA(OOP_OCLASS(gfx), gfx);
    output_ids[0] = ((drmModeConnectorPtr)gfxdata->selectedconnector)->connector_id;
    

    ret = drmModeSetCrtc(nvdev->fd, gfxdata->selectedcrtcid,
	        bmdata->fbid, -bmdata->xoffset, -bmdata->yoffset, output_ids, 
	        output_count, gfxdata->selectedmode);

    UNLOCK_BITMAP
    
    if (ret) return FALSE; else return TRUE;
}

static BOOL HIDDNouveauSwitchToVideoMode(OOP_Object * bm)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);
    OOP_Object * gfx = NULL;
    struct HIDDNouveauData * gfxdata = NULL; 
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device_priv *nvdev = nouveau_device(carddata->dev);
    LONG i;
    drmModeConnectorPtr selectedconnector = NULL;
    HIDDT_ModeID modeid;
    OOP_Object * sync;
    OOP_Object * pf;
    IPTR pixel, e;
    IPTR hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;
    LONG ret;
    
    
    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);
    gfx = (OOP_Object *)e;
    gfxdata = OOP_INST_DATA(OOP_OCLASS(gfx), gfx);
    selectedconnector = (drmModeConnectorPtr)gfxdata->selectedconnector;

    D(bug("[Nouveau] HIDDNouveauSwitchToVideoMode\n"));
    
    /* We should be able to get modeID from the bitmap */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);

    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[Nouveau] Invalid ModeID\n"));
        return FALSE;
    }

    /* Get Sync and PixelFormat properties */
    struct pHidd_Gfx_GetMode __getmodemsg = 
    {
        modeID:	modeid,
        syncPtr:	&sync,
        pixFmtPtr:	&pf,
    }, *getmodemsg = &__getmodemsg;

    getmodemsg->mID = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_GetMode);
    OOP_DoMethod(gfx, (OOP_Msg)getmodemsg);

    OOP_GetAttr(sync, aHidd_Sync_PixelClock,    &pixel);
    OOP_GetAttr(sync, aHidd_Sync_HDisp,         &hdisp);
    OOP_GetAttr(sync, aHidd_Sync_VDisp,         &vdisp);
    OOP_GetAttr(sync, aHidd_Sync_HSyncStart,    &hstart);
    OOP_GetAttr(sync, aHidd_Sync_VSyncStart,    &vstart);
    OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,      &hend);
    OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,      &vend);
    OOP_GetAttr(sync, aHidd_Sync_HTotal,        &htotal);
    OOP_GetAttr(sync, aHidd_Sync_VTotal,        &vtotal);    
    
    D(bug("[Nouveau] Sync: %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
    pixel, hdisp, hstart, hend, htotal, vdisp, vstart, vend, vtotal));

    D(bug("[Nouveau] Connector %d, CRTC %d\n", 
        selectedconnector->connector_id, gfxdata->selectedcrtcid));

    /* Select mode */
    gfxdata->selectedmode = NULL;
    for (i = 0; i < selectedconnector->count_modes; i++)
    {
        drmModeModeInfoPtr mode = &selectedconnector->modes[i];
        
        if ((mode->hdisplay == hdisp) && (mode->vdisplay == vdisp) &&
            (mode->hsync_start == hstart) && (mode->vsync_start == vstart) &&
            (mode->hsync_end == hend) && (mode->vsync_end == vend))
        {
            gfxdata->selectedmode = mode;
            break;
        }
    }
    
    if (!gfxdata->selectedmode)
    {
        D(bug("[Nouveau] Not able to select mode\n"));
        return FALSE;
    }

    /* Set the displayed width and height of bitmap */
    bmdata->displayedwidth = hdisp;
    bmdata->displayedheight = vdisp;


    /* For screen switching the bitmap might have already once been a framebuffer 
       - check bmdata->fbid. Also the bitmap itself needs to know whether it is 
       added as framebuffer so that it can unregister itself in Dispose */

    /* Add as frame buffer */
    if (bmdata->fbid == 0)
    {
	    ret = drmModeAddFB(nvdev->fd, bmdata->width, bmdata->height, 
	                bmdata->depth, bmdata->bytesperpixel * 8, 
	                bmdata->pitch, bmdata->bo->handle, &bmdata->fbid);
        if (ret)
        {
            D(bug("[Nouveau] Not able to add framebuffer\n"));
            return FALSE;
        }
    }


    /* Switch mode */
    if (!HIDDNouveauShowBitmapForSelectedMode(bm))
    {
        D(bug("[Nouveau] Not able to set crtc\n"));
        return FALSE;        
    }

    HIDDNouveauShowCursor(gfx, TRUE);
        
    return TRUE;
}

#if ENABLE_COMPOSITING
/* struct Rectangle and AndRectRect are present in graphics.library.
   They are "doubled" here so that there is no dependency from nouveau.hidd
   to graphics.library */

struct Rectangle
{
    WORD MinX;
    WORD MinY;
    WORD MaxX;
    WORD MaxY;
};

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

/* Information related to current screen mode */
OOP_Object *        screenbitmap = NULL;
HIDDT_ModeID        screenmodeid = vHidd_ModeID_Invalid;
struct Rectangle    screenrect;

/* This is z-ordered stack of visible bitmaps */
struct List bitmapstack;

struct StackBitMapNode
{
    struct Node         n;
    OOP_Object *        bm;
    struct Rectangle    screenvisiblerect;
    BOOL                isscreenvisible;
};

static VOID Compositing_RecalculateVisibleRects()
{
    ULONG lastscreenvisibleline = screenrect.MaxY;
    struct StackBitMapNode * n = NULL;
    
    /* TODO: probably add composing wide read lock */
    
    ForeachNode(&bitmapstack, n)
    {
        /*  Stack bitmap bounding boxes equal screen bounding box taking into
            account topedge */
        IPTR topedge;
        struct Rectangle tmprect;
        OOP_Class * cl = OOP_OCLASS(n->bm);
        
        OOP_GetAttr(n->bm, aHidd_BitMap_TopEdge, &topedge);
        /* Copy screen rect */
        tmprect = screenrect;
        /* Set bottom and top values */
        tmprect.MinY = topedge;
        tmprect.MaxY = lastscreenvisibleline;
        /* Intersect both to make sure values are withint screen limit */
        if (AndRectRect(&tmprect, &screenrect, &n->screenvisiblerect))
        {
            lastscreenvisibleline = n->screenvisiblerect.MinY;
            n->isscreenvisible = TRUE;
        }
        else
            n->isscreenvisible = FALSE;

        bug("Bitmap %x, visible %d, (%d, %d) , (%d, %d)\n", n->bm, n->isscreenvisible, 
            n->screenvisiblerect.MinX, n->screenvisiblerect.MinY, 
            n->screenvisiblerect.MaxX, n->screenvisiblerect.MaxY);
    }
}

static VOID Compositing_RecalculateDisplayedWidthHeight()
{
    OOP_Class * cl = OOP_OCLASS(screenbitmap);
    struct StackBitMapNode * n = NULL;
    IPTR displayedwidth, displayedheight;

    /* TODO: probably add composing wide read lock */
    
    OOP_GetAttr(screenbitmap, aHidd_BitMap_Width, &displayedwidth);
    OOP_GetAttr(screenbitmap, aHidd_BitMap_Height, &displayedheight);
    
    ForeachNode(&bitmapstack, n)
    {
        struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(OOP_OCLASS(n->bm), n->bm);
        bmdata->displayedwidth  = displayedwidth;
        bmdata->displayedheight = displayedheight;
    }
}

static struct StackBitMapNode * Compositing_IsBitMapOnStack(OOP_Object * bm)
{
    struct StackBitMapNode * n = NULL;
    
    /* TODO: probably add composing wide read lock */

    ForeachNode(&bitmapstack, n)
    {
        if (n->bm == bm)
            return n;
    }

    return NULL;
}

static BOOL Compositing_CanCompositeWithScreenBitMap(OOP_Object * bm)
{
    OOP_Object * screenbm = screenbitmap;
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

#endif

BOOL Compositing_TopBitMapChanged(OOP_Object * bm)
{
#if ENABLE_COMPOSITING
    /* 
        Desctiption: 
        take incomming top bitmap
        read its mode and sizes,
        create a mirroring bitmap that fits the mode
        switch mode 
    */
    /* TODO: needs locking mechanism */
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
    if (modeid == screenmodeid)
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
            HIDD_Gfx_DisposeBitMap(gfx, screenbitmap);

            /* Store bitmap/mode information */ 
            screenmodeid    = modeid;
            screenbitmap    = fbbitmap;
            screenrect.MinX = 0;
            screenrect.MinY = 0;
            screenrect.MaxX = hdisp - 1;
            screenrect.MaxY = vdisp - 1;
            
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
#else
    return HIDDNouveauSwitchToVideoMode(bm);
#endif
}

VOID Compositing_BitMapStackChanged(struct HIDD_ViewPortData * vpdata)
{
#if ENABLE_COMPOSITING
    struct HIDD_ViewPortData * vp;
    /* TODO: probably needs driver wide lock */
    /* TODO: free all items which are already on the list */
    NEWLIST(&bitmapstack); /* YES THIS IS MEMORY LEAK */
    
    
    if (!vpdata)
        return; /* TODO: BLANK SCREEN */

    /* Switch mode if needed */    
    Compositing_TopBitMapChanged(vpdata->Bitmap);
    
    /* Copy bitmaps pointers to our stack */
    for (vp = vpdata; vp; vp = vp->Next)
    {
        /* Check if the passed bitmap can be composited together with screen
           bitmap */
        if (Compositing_CanCompositeWithScreenBitMap(vp->Bitmap))
        {
            struct StackBitMapNode * n = AllocVec(sizeof(struct StackBitMapNode), MEMF_ANY | MEMF_CLEAR);
            n->bm = vp->Bitmap;
            n->isscreenvisible = FALSE;
            AddTail(&bitmapstack, (struct Node *)n);
        }
    }

    /* Recalculate visible rects per screen */
    Compositing_RecalculateVisibleRects();
    
    /* Set displayedwidth/displayedheight on all screen bitmaps */
    Compositing_RecalculateDisplayedWidthHeight();

    Compositing_BitMapPositionChanged(vpdata->Bitmap);
#endif   
}


/* Assumes LeftEdge and TopEdge are already set to new values */
BOOL Compositing_BitMapPositionChanged(OOP_Object * bm)
{
#if ENABLE_COMPOSITING
    /* TODO:probabaly needs a compositing wide read lock */
    struct StackBitMapNode * n = NULL;
    ULONG lastscreenvisibleline = screenrect.MaxY;
    
    /* Check is passed bitmap is in stack, ignore if not */
    if ((n = Compositing_IsBitMapOnStack(bm)) == NULL)
        return TRUE;
    
    /* Recalculate visible rects per screen */
    Compositing_RecalculateVisibleRects();
    
    /* TODO: Optimization - if top bitmap covers the complete physical screen,
       the driver can turn off mirroring mode and use the top bitmap as
       framebuffer */

    /* Refresh all bitmaps on stack */
    ForeachNode(&bitmapstack, n)
    {
        if (n->isscreenvisible)
        {
            OOP_Class * cl = OOP_OCLASS(n->bm);
            IPTR width, height;
            OOP_GetAttr(n->bm, aHidd_BitMap_Width, &width);
            OOP_GetAttr(n->bm, aHidd_BitMap_Height, &height);
            Compositing_BitMapRectChanged(n->bm, 0, 0, width, height);
            if (lastscreenvisibleline > n->screenvisiblerect.MinY)
                lastscreenvisibleline = n->screenvisiblerect.MinY;
        }
    }

    /* Clean up area revealed by drag */
    /* TODO: Find all areas which might have been releaved, not only top - 
       This will happen when there are bitmaps of different sizes composited */
    if (lastscreenvisibleline > 0)
    {
        OOP_Class * cl = OOP_OCLASS(screenbitmap);
        IPTR e, viswidth;
        OOP_Object * gfx = NULL;
        OOP_Object * tmpgc = NULL;
        struct TagItem gctags[] =
        {
            { aHidd_GC_Foreground, (HIDDT_Pixel)0x99999999 }, /* TODO: value depends on depth */
            { TAG_DONE, TAG_DONE }
        };
        
        OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);
        gfx = (OOP_Object *)e;
        OOP_GetAttr(screenbitmap, aHidd_BitMap_Width, &viswidth); 

        /* TODO: Cache this option in gfxdata */
        tmpgc = HIDD_Gfx_NewGC(gfx, gctags);
        HIDD_BM_FillRect(screenbitmap, tmpgc, 0, 0, viswidth, lastscreenvisibleline);
        HIDD_Gfx_DisposeGC(gfx, tmpgc);
    }
    

    return TRUE;
#else
    return HIDDNouveauShowBitmapForSelectedMode(bm);
#endif
}

VOID Compositing_BitMapRectChanged(OOP_Object * bm, WORD x, WORD y, WORD width, WORD height)
{
#if ENABLE_COMPOSITING
    /* TODO:probabaly needs a compositing wide read lock */
    struct StackBitMapNode * n = NULL;
    
    /* Check if passed bitmap is in stack, ignore if not */
    if ((n = Compositing_IsBitMapOnStack(bm)) == NULL)
        return;

    if (!n->isscreenvisible)
        return;

    if (screenbitmap)
    {
        OOP_Class * cl = OOP_OCLASS(bm);
        OOP_Object * gfx = NULL;
        OOP_Object * tmpgc = NULL;
        IPTR e, leftedge, topedge;
        struct Rectangle srcrect;
        struct Rectangle srcindstrect;
        struct Rectangle dstandvisrect;

        OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);
        gfx = (OOP_Object *)e;
        OOP_GetAttr(bm, aHidd_BitMap_LeftEdge, &leftedge);
        OOP_GetAttr(bm, aHidd_BitMap_TopEdge, &topedge);
        
        /* Rectangle in source bitmap coord system */
        srcrect.MinX = x; srcrect.MinY = y;
        srcrect.MaxX = x + width - 1; srcrect.MaxY = y + height - 1;
        
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
                screenbitmap,
                dstandvisrect.MinX, dstandvisrect.MinY,
                dstandvisrect.MaxX - dstandvisrect.MinX + 1,
                dstandvisrect.MaxY - dstandvisrect.MinY + 1,
                tmpgc);
            HIDD_Gfx_DisposeGC(gfx, tmpgc);
        }
    }
#endif
}



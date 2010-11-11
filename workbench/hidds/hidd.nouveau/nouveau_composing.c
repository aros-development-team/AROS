/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"
#include "nouveau_composing.h"

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

#if ENABLE_COMPOSING
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

OOP_Object * visiblebitmap = NULL;
HIDDT_ModeID visiblemodeid = vHidd_ModeID_Invalid;
struct Rectangle visiblerect;

OOP_Object * topbitmap = NULL;
#endif

BOOL Composing_TopBitMapChanged(OOP_Object * bm)
{
#if ENABLE_COMPOSING
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
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);

    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);
    gfx = (OOP_Object *)e;

    /* Read display mode properties */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);
    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[Nouveau] Invalid ModeID\n"));
        return FALSE;
    }
    
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

    /* If the mode is already visible, just switch top bitmap and provide limits */
    if (modeid == visiblemodeid)
    {
        topbitmap               = bm;
        bmdata->displayedwidth  = hdisp;
        bmdata->displayedheight = vdisp;
        return TRUE;
    }

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
            /* Dispose the previous visiblebitmap */
            HIDD_Gfx_DisposeBitMap(gfx, visiblebitmap);

            /* Store bitmap/mode information */ 
            topbitmap           = bm;
            visiblemodeid       = modeid;
            visiblebitmap       = fbbitmap;
            visiblerect.MinX    = 0;
            visiblerect.MinY    = 0;
            visiblerect.MaxX    = hdisp - 1;
            visiblerect.MaxY    = vdisp - 1;
            
            /* Update displayed width/height for input bitmap */
            bmdata->displayedwidth      = hdisp;
            bmdata->displayedheight     = vdisp;
            return TRUE;
        }
        else
        {
            /* Dispose fbbitmap */
            HIDD_Gfx_DisposeBitMap(gfx, fbbitmap);
            return FALSE;
        }
    }

    /* TODO: probably needs to call Composing_BitMapPositionChanged to make sure
        revealed part of screen is erased (passed bitmap can be already lowered)
        Call needs to happen in case of same mode as well */
    return FALSE;
#else
    return HIDDNouveauSwitchToVideoMode(bm);
#endif
}

/* Assumes LeftEdge and TopEdge are already set to new values */
BOOL Composing_BitMapPositionChanged(OOP_Object * bm)
{
#if ENABLE_COMPOSING
    /* Reblit the complete bitmap */
    OOP_Class * cl = OOP_OCLASS(bm);
    IPTR width, height, topedge;
    
    if (bm != topbitmap)
        return TRUE; /* Ignore */
    
    OOP_GetAttr(bm, aHidd_BitMap_Width, &width);
    OOP_GetAttr(bm, aHidd_BitMap_Height, &height);
    OOP_GetAttr(bm, aHidd_BitMap_TopEdge, &topedge);
    
    /* Clean up area revealed by drag */
    if (topedge > 0)
    {
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
        OOP_GetAttr(visiblebitmap, aHidd_BitMap_Width, &viswidth); 

        /* TODO: Cache this option in gfxdata */
        tmpgc = HIDD_Gfx_NewGC(gfx, gctags);
        HIDD_BM_FillRect(visiblebitmap, tmpgc, 0, 0, viswidth, topedge);
        HIDD_Gfx_DisposeGC(gfx, tmpgc);
    }
    
    Composing_BitMapRectChanged(bm, 0, 0, width, height);

    return TRUE;
#else
    return HIDDNouveauShowBitmapForSelectedMode(bm);
#endif
}

VOID Composing_BitMapRectChanged(OOP_Object * bm, WORD x, WORD y, WORD width, WORD height)
{
#if ENABLE_COMPOSING
    if (bm != topbitmap)
        return;

    if (visiblebitmap)
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
        
        /* Find intersection of screen rect and srcindst rect */
        if (AndRectRect(&srcindstrect, &visiblerect, &dstandvisrect))
        {
            /* Intersection is valid. Blit. */        
            /* TODO: stubs have performance hit, change to direct DoMethod call with cached MethodID */

            /* TODO: Cache this option in gfxdata */
            tmpgc = HIDD_Gfx_NewGC(gfx, NULL);        
            HIDD_Gfx_CopyBox(gfx,
                bm,
                /* Transform back to source bitmap coord system */
                dstandvisrect.MinX - leftedge, dstandvisrect.MinY - topedge,
                visiblebitmap,
                dstandvisrect.MinX, dstandvisrect.MinY,
                dstandvisrect.MaxX - dstandvisrect.MinX + 1,
                dstandvisrect.MaxY - dstandvisrect.MinY + 1,
                tmpgc);
            HIDD_Gfx_DisposeGC(gfx, tmpgc);
        }
    }
#endif
}


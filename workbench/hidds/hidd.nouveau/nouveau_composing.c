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
#define HiddPixFmtAttrBase  (SD(cl)->pixFmtAttrBase)
#define HiddGfxAttrBase     (SD(cl)->gfxAttrBase)
#define HiddSyncAttrBase    (SD(cl)->syncAttrBase)
#define HiddBitMapAttrBase  (SD(cl)->bitMapAttrBase)

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
OOP_Object * visiblebitmap = NULL;
HIDDT_ModeID visiblemodeid = vHidd_ModeID_Invalid;
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

    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);
    gfx = (OOP_Object *)e;

    /* Read display mode properties */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);
    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[Nouveau] Invalid ModeID\n"));
        return FALSE;
    }
    
    /* If the mode is alrady visible, just switch top bitmap */
    if (modeid == visiblemodeid)
    {
        topbitmap = bm;
        return TRUE;
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

    bug("TBCH: %d, %d, %d\n", modeid, hdisp, vdisp);
    
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
            topbitmap       = bm;
            visiblemodeid   = modeid;
            visiblebitmap   = fbbitmap;
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

BOOL Composing_BitMapPositionChanged(OOP_Object * bm)
{
#if ENABLE_COMPOSING
    return FALSE;
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
        IPTR e;

        OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);
        gfx = (OOP_Object *)e;
        
        /* TODO: stubs have performance hit, change to direct DoMethod call with cached MethodID */
        tmpgc = HIDD_Gfx_NewGC(gfx, NULL);        
        HIDD_Gfx_CopyBox(gfx, bm, x, y, visiblebitmap, x, y, width, height, tmpgc);
        HIDD_Gfx_DisposeGC(gfx, tmpgc);
    }
#endif
}


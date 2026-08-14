/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.
*/

#include "nouveau_intern.h"
#include "compositor.h"

#include <graphics/displayinfo.h>
#include <proto/utility.h>

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>

#include <libdrm/arosdrmmode.h>

#undef HiddAttrBase
#undef HiddPixFmtAttrBase
#undef HiddGfxAttrBase
#undef HiddDisplayAttrBase
#undef HiddDMEnumAttrBase
#undef HiddGfxNouveauAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#undef HiddCompositorAttrBase
#undef HiddBitMapNouveauAttrBase

#define HiddAttrBase          (SD(cl)->hiddAttrBase)
#define HiddPixFmtAttrBase          (SD(cl)->pixFmtAttrBase)
#define HiddGfxAttrBase             (SD(cl)->gfxAttrBase)
#define HiddDisplayAttrBase         (SD(cl)->displayAttrBase)
#define HiddDMEnumAttrBase          (SD(cl)->dmenumAttrBase)
#define HiddGfxNouveauAttrBase      (SD(cl)->gfxNouveauAttrBase)
#define HiddSyncAttrBase            (SD(cl)->syncAttrBase)
#define HiddBitMapAttrBase          (SD(cl)->bitMapAttrBase)
#define HiddCompositorAttrBase     (SD(cl)->compositorAttrBase)
#define HiddBitMapNouveauAttrBase   (SD(cl)->bitMapNouveauAttrBase)

/* HELPER FUNCTIONS */
VOID HIDDNouveauShowCursor(OOP_Object * gfx, BOOL visible)
{
    OOP_Class * cl = OOP_OCLASS(gfx);
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, gfx);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;

    LOCK_ENGINE

    if (visible)
    {
        drmModeSetCursor(nvdev->fd, gfxdata->selectedcrtcid, 
            gfxdata->cursor->handle, 64, 64);
    }
    else
    {
        drmModeSetCursor(nvdev->fd, gfxdata->selectedcrtcid, 
            0, 64, 64);
    }

    UNLOCK_ENGINE
}

/* This function assumes that the mode, crtc and output are already selected */
static BOOL HIDDNouveauShowBitmapForSelectedMode(OOP_Object * bm)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauData * gfxdata = NULL;
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;
    uint32_t output_ids[] = {0};
    uint32_t output_count = 1;
    IPTR e = (IPTR)NULL;
    OOP_Object * gfx = NULL;
    LONG ret;

    LOCK_ENGINE
    LOCK_BITMAP
    
    /* Check if passed bitmap has been registered as framebuffer */
    if (bmdata->fbid == 0)
    {
        UNLOCK_BITMAP
        UNLOCK_ENGINE
        return FALSE;
    }
    
    OOP_GetAttr(bm, aHidd_BitMap_Display, &e);
    OOP_GetAttr((OOP_Object *)e, aHidd_Display_GfxHidd, &e);
    gfx = (OOP_Object *)e;
    gfxdata = OOP_INST_DATA(OOP_OCLASS(gfx), gfx);
    output_ids[0] = ((drmModeConnectorPtr)gfxdata->selectedconnector)->connector_id;
    

    ret = drmModeSetCrtc(nvdev->fd, gfxdata->selectedcrtcid,
            bmdata->fbid, -bmdata->xoffset, -bmdata->yoffset, output_ids,
            output_count, gfxdata->selectedmode);

    UNLOCK_BITMAP
    UNLOCK_ENGINE

    if (ret) return FALSE; else return TRUE;
}

BOOL HIDDNouveauSwitchToVideoMode(OOP_Object * bm)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);
    OOP_Object * gfx = NULL;
    struct HIDDNouveauData * gfxdata = NULL; 
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;
    LONG i;
    drmModeConnectorPtr selectedconnector = NULL;
    HIDDT_ModeID modeid;
    OOP_Object * sync;
    OOP_Object * pf;
    IPTR pixel, e;
    IPTR hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;
    LONG ret;

    LOCK_ENGINE

    OOP_GetAttr(bm, aHidd_BitMap_Display, &e);
    OOP_GetAttr((OOP_Object *)e, aHidd_Display_GfxHidd, &e);
    gfx = (OOP_Object *)e;
    gfxdata = OOP_INST_DATA(OOP_OCLASS(gfx), gfx);
    selectedconnector = (drmModeConnectorPtr)gfxdata->selectedconnector;

    D(bug("[Nouveau] HIDDNouveauSwitchToVideoMode, bm: 0x%x\n", bm));
    
    /* We should be able to get modeID from the bitmap */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);

    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[Nouveau] Invalid ModeID\n"));
        UNLOCK_ENGINE
        return FALSE;
    }

    /* Get Sync and PixelFormat properties */
    HIDD_DMEnum_GetMode(SD(cl)->dmenum, modeid, &sync, &pf);

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
        UNLOCK_ENGINE
        return FALSE;
    }

    /* For screen switching the bitmap might have already once been a framebuffer 
       - check bmdata->fbid. Also the bitmap itself needs to know whether it is 
       added as framebuffer so that it can unregister itself in Dispose */

    /* Add as frame buffer */
    if (bmdata->fbid == 0)
    {
	    ret = drmModeAddFB(nvdev->fd, bmdata->drawable.width, bmdata->drawable.height,
	                bmdata->drawable.depth, bmdata->bytesperpixel * 8,
	                bmdata->pitch, bmdata->bo->handle, &bmdata->fbid);
        if (ret)
        {
            D(bug("[Nouveau] Not able to add framebuffer\n"));
            UNLOCK_ENGINE
            return FALSE;
        }
    }


    /* Switch mode */
    if (!HIDDNouveauShowBitmapForSelectedMode(bm))
    {
        D(bug("[Nouveau] Not able to set crtc\n"));
        UNLOCK_ENGINE
        return FALSE;        
    }

    HIDDNouveauShowCursor(gfx, TRUE);

    UNLOCK_ENGINE
    return TRUE;
}

/* PUBLIC METHODS */
VOID METHOD(NouveauDisplay, Root, Get)
{
    ULONG idx;

    Hidd_Display_Switch(msg->attrID, idx)
    {
    case aoHidd_Display_SpriteTypes:
        *msg->storage = vHidd_SpriteType_DirectColor;
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object * METHOD(NouveauDisplay, Hidd_Display, CreateObject)
{
    OOP_Object      *object = NULL;

    if (msg->cl == SD(cl)->basebm)
    {
        struct pHidd_Display_CreateObject mymsg;
        HIDDT_ModeID modeid;
        HIDDT_StdPixFmt stdpf;

        struct TagItem mytags [] =
        {
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_ClassPtr */
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_Align */
            { aHidd_BitMap_Nouveau_CompositorHidd, (IPTR)SD(cl)->compositor },
            { TAG_MORE, (IPTR)msg->attrList }
        };

        /* Check if user provided valid ModeID */
        /* Check for framebuffer - not needed as Nouveau is a NoFramebuffer driver */
        /* Check for displayable - not needed - displayable has ModeID and we don't
           distinguish between on-screen and off-screen bitmaps */
        modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        if (vHidd_ModeID_Invalid != modeid) 
        {
            /* User supplied a valid modeid. We can use our bitmap class */
            mytags[0].ti_Tag	= aHidd_BitMap_ClassPtr;
            mytags[0].ti_Data	= (IPTR)SD(cl)->bmclass;
        } 

        /* Check if bitmap is a planar bitmap */
        stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
        if (vHidd_StdPixFmt_Plane == stdpf)
        {
            mytags[1].ti_Tag    = aHidd_BitMap_Align;
            mytags[1].ti_Data   = 32;
        }
        
        /* We init a new message struct */
        mymsg.mID	= msg->mID;
        mymsg.cl	= msg->cl;
        mymsg.attrList	= mytags;

        /* Pass the new message to the superclass */
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    }
    else if (SD(cl)->basegallium && (msg->cl == SD(cl)->basegallium))
    {
        /* Create the gallium 3d driver object .. */
        object = OOP_NewObject(NULL, CLID_Hidd_Gallium_Nouveau, msg->attrList);
    }
    else if (SD(cl)->basei2c && (msg->cl == SD(cl)->basei2c))
    {
        /* Expose the i2c bus object .. */
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return object;
}

ULONG METHOD(NouveauDisplay, Hidd_Display, ShowViewPorts)
{
    struct pHidd_Compositor_BitMapStackChanged bscmsg =
    {
        .mID  = OOP_GetMethodID(IID_Hidd_Compositor, moHidd_Compositor_BitMapStackChanged),
        .data  = msg->Data
    };

    D(bug("[Nouveau] ShowViewPorts enter TopLevelBM %x\n", (msg->Data ? (msg->Data->Bitmap) : NULL)));

    OOP_DoMethod(SD(cl)->compositor, (OOP_Msg)&bscmsg);

    return TRUE; /* Indicate driver supports this method */
}

#if AROS_BIG_ENDIAN
#define Machine_ARGB32 vHidd_StdPixFmt_ARGB32
#else
#define Machine_ARGB32 vHidd_StdPixFmt_BGRA32
#endif

BOOL METHOD(NouveauDisplay, Hidd_Display, SetCursorShape)
{
    OOP_Object *gfx = NULL;
    struct HIDDNouveauData * gfxdata;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    gfxdata = OOP_INST_DATA(SD(cl)->gfxclass, gfx);

    if (msg->shape == NULL)
    {
        /* Hide cursor */
        HIDDNouveauShowCursor(gfx, FALSE);
    }
    else
    {
        IPTR width, height;
        ULONG i;
        ULONG x, y;
        ULONG curimage[64 * 64];
        struct CardData * carddata = &(SD(cl)->carddata);
        
        OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
        OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);


        if (width > 64) width = 64;
        if (height > 64) height = 64;

        LOCK_ENGINE

        /* Map the cursor buffer */
        nouveau_bo_map(gfxdata->cursor, NOUVEAU_BO_WR, carddata->client);

        /* Clear the matrix */
        for (i = 0; i < 64 * 64; i++)
            ((ULONG*)gfxdata->cursor->map)[i] = 0;

        /* Get data from the bitmap */
        HIDD_BM_GetImage(msg->shape, (UBYTE *)curimage, 64 * 4, 0, 0, 
            width, height, Machine_ARGB32);
        
        if (carddata->Architecture < NV_TESLA)
        {
            ULONG offset, pixel, blue, green, red, alpha;

            /* The image needs to be premultiplied */
            for (y = 0; y < height; y++)
                for (x = 0; x < width; x++)
                {
                    offset = y * 64 + x;
                    pixel = curimage[offset];
                    blue  = (pixel & 0x000000FF);
                    green = (pixel & 0x0000FF00) >> 8;
                    red   = (pixel & 0x00FF0000) >> 16;
                    alpha = (pixel & 0xFF000000) >> 24;
                    
                    blue    = (blue * alpha) / 255;
                    green   = (green * alpha) / 255;
                    red     = (red * alpha) / 255;

                    curimage[offset]    = (alpha << 24) | (red << 16) | (green << 8) | blue;
                }
        }

        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
            {
                ULONG offset = y * 64 + x;
                writel(curimage[offset], ((ULONG *)gfxdata->cursor->map) + (offset));
            }

        /* Show updated cursor */
        HIDDNouveauShowCursor(gfx, TRUE);

        UNLOCK_ENGINE
    }

    return TRUE;
}

BOOL METHOD(NouveauDisplay, Hidd_Display, SetCursorPos)
{
    OOP_Object *gfx = NULL;
    struct HIDDNouveauData * gfxdata;
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    gfxdata = OOP_INST_DATA(SD(cl)->gfxclass, gfx);

    LOCK_ENGINE
    drmModeMoveCursor(nvdev->fd, gfxdata->selectedcrtcid, msg->x, msg->y);
    UNLOCK_ENGINE

    return TRUE;
}

VOID METHOD(NouveauDisplay, Hidd_Display, SetCursorVisible)
{
    OOP_Object *gfx = NULL;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    HIDDNouveauShowCursor(gfx, msg->visible);
}

static struct HIDD_ModeProperties modeprops = 
{
    DIPF_IS_SPRITES,
    1,
    COMPF_ABOVE
};

ULONG METHOD(NouveauDisplay, Hidd_Display, ModeProperties)
{
    ULONG len = msg->propsLen;

    if (len > sizeof(modeprops))
        len = sizeof(modeprops);
    CopyMem(&modeprops, msg->props, len);

    return len;
}

/*
 * What size of screen to open when nothing asks for a particular one.
 *
 * The monitor says which timing it was built for, and that is the one
 * it displays without complaint; anything else it merely tolerates, if
 * at all. Take its dimensions from that mode rather than from a fixed
 * pair, so the default screen is the one the display actually wants.
 */
VOID METHOD(NouveauDisplay, Hidd_Display, NominalDimensions)
{
    OOP_Object *gfx = NULL;
    ULONG width = 1024, height = 768;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    if (gfx)
    {
        struct HIDDNouveauData *gfxdata = OOP_INST_DATA(SD(cl)->gfxclass, gfx);
        drmModeConnectorPtr connector = (drmModeConnectorPtr)gfxdata->selectedconnector;

        if (connector && connector->count_modes > 0)
        {
            drmModeModeInfoPtr mode = &connector->modes[0];
            int i;

            for (i = 0; i < connector->count_modes; i++)
            {
                if (connector->modes[i].type & DRM_MODE_TYPE_PREFERRED)
                {
                    mode = &connector->modes[i];
                    break;
                }
            }

            /* Nothing marked preferred: the list is ordered best first */
            width = mode->hdisplay;
            height = mode->vdisplay;

            D(bug("[Nouveau] Nominal dimensions %ux%u from '%s'\n",
                  width, height, mode->name));
        }
    }

    if (msg->width)
        *(msg->width) = width;
    if (msg->height)
        *(msg->height) = height;
    if (msg->depth)
        *(msg->depth) = 24;
}

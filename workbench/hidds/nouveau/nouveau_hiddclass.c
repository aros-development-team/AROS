/*
    Copyright © 2010-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"
#include "compositor.h"

#include <graphics/displayinfo.h>
#include <proto/utility.h>
#include <nouveau_drm.h>

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>

#include "arosdrmmode.h"

#undef HiddAttrBase
#undef HiddPixFmtAttrBase
#undef HiddGfxAttrBase
#undef HiddGfxNouveauAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#undef HiddCompositorAttrBase
#undef HiddBitMapNouveauAttrBase

#define HiddAttrBase          (SD(cl)->hiddAttrBase)
#define HiddPixFmtAttrBase          (SD(cl)->pixFmtAttrBase)
#define HiddGfxAttrBase             (SD(cl)->gfxAttrBase)
#define HiddGfxNouveauAttrBase      (SD(cl)->gfxNouveauAttrBase)
#define HiddSyncAttrBase            (SD(cl)->syncAttrBase)
#define HiddBitMapAttrBase          (SD(cl)->bitMapAttrBase)
#define HiddCompositorAttrBase     (SD(cl)->compositorAttrBase)
#define HiddBitMapNouveauAttrBase   (SD(cl)->bitMapNouveauAttrBase)

#define MAX_BITMAP_WIDTH    4096
#define MAX_BITMAP_HEIGHT   4096
#define GART_BUFFER_SIZE    (12 * 1024 * 1024)

/* HELPER FUNCTIONS */
VOID HIDDNouveauShowCursor(OOP_Object * gfx, BOOL visible)
{
    OOP_Class * cl = OOP_OCLASS(gfx);
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, gfx);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device_priv * nvdev = nouveau_device(carddata->dev);

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

static BOOL HIDDNouveauSelectConnectorCrtc(LONG fd, drmModeConnectorPtr * selectedconnector,
    drmModeCrtcPtr * selectedcrtc)
{
    *selectedconnector = NULL;
    *selectedcrtc = NULL;
    drmModeResPtr drmmode = NULL;
    LONG i; ULONG crtc_id;

    LOCK_ENGINE

    /* Get all components information */
    drmmode = drmModeGetResources(fd);
    if (!drmmode)
    {
        D(bug("[Nouveau] Not able to get resources information\n"));
        UNLOCK_ENGINE
        return FALSE;
    }
    
    /* Selecting connector */
    for (i = 0; i < drmmode->count_connectors; i++)
    {
        drmModeConnectorPtr connector = drmModeGetConnector(fd, drmmode->connectors[i]);

        if (connector)
        {
            if (connector->connection == DRM_MODE_CONNECTED)
            {
                /* Found connected connector */
                *selectedconnector = connector;
                break;
            }
            
            drmModeFreeConnector(connector);
        }
    }
    
    if (!(*selectedconnector))
    {
        D(bug("[Nouveau] No connected connector\n"));
        drmModeFreeResources(drmmode);
        UNLOCK_ENGINE
        return FALSE;
    }

    /* Selecting first available CRTC */
    if (drmmode->count_crtcs > 0)
        crtc_id = drmmode->crtcs[0];
    else
        crtc_id = 0;

    *selectedcrtc = drmModeGetCrtc(fd, crtc_id);
    if (!(*selectedcrtc))
    {
        D(bug("[Nouveau] Not able to get crtc information for crtc_id %d\n", crtc_id));
        drmModeFreeConnector(*selectedconnector);
        *selectedconnector = NULL;
        drmModeFreeResources(drmmode);
        UNLOCK_ENGINE
        return FALSE;
    }
    
    drmModeFreeResources(drmmode);
    UNLOCK_ENGINE
    return TRUE;
}    

#include <stdio.h>

static struct TagItem * HIDDNouveauCreateSyncTagsFromConnector(OOP_Class * cl, drmModeConnectorPtr connector)
{
    struct TagItem * syncs = NULL;
    ULONG modescount = connector->count_modes;
    ULONG i;
    
    if (modescount == 0)
        return NULL;
        
    /* Allocate enough structures */
    syncs = HIDDNouveauAlloc(sizeof(struct TagItem) * modescount);
    
    for (i = 0; i < modescount; i++)
    {
        struct TagItem * sync = HIDDNouveauAlloc(sizeof(struct TagItem) * 15);
        LONG j = 0;
        
        drmModeModeInfoPtr mode = &connector->modes[i];

        sync[j].ti_Tag = aHidd_Sync_PixelClock;     sync[j++].ti_Data = mode->clock;

        sync[j].ti_Tag = aHidd_Sync_HDisp;          sync[j++].ti_Data = mode->hdisplay;
        sync[j].ti_Tag = aHidd_Sync_HSyncStart;     sync[j++].ti_Data = mode->hsync_start;
        sync[j].ti_Tag = aHidd_Sync_HSyncEnd;       sync[j++].ti_Data = mode->hsync_end;
        sync[j].ti_Tag = aHidd_Sync_HTotal;         sync[j++].ti_Data = mode->htotal;
        sync[j].ti_Tag = aHidd_Sync_HMin;           sync[j++].ti_Data = mode->hdisplay;
        sync[j].ti_Tag = aHidd_Sync_HMax;           sync[j++].ti_Data = MAX_BITMAP_WIDTH;

        sync[j].ti_Tag = aHidd_Sync_VDisp;          sync[j++].ti_Data = mode->vdisplay;
        sync[j].ti_Tag = aHidd_Sync_VSyncStart;     sync[j++].ti_Data = mode->vsync_start;
        sync[j].ti_Tag = aHidd_Sync_VSyncEnd;       sync[j++].ti_Data = mode->vsync_end;
        sync[j].ti_Tag = aHidd_Sync_VTotal;         sync[j++].ti_Data = mode->vtotal;
        sync[j].ti_Tag = aHidd_Sync_VMin;           sync[j++].ti_Data = mode->vdisplay;
        sync[j].ti_Tag = aHidd_Sync_VMax;           sync[j++].ti_Data = MAX_BITMAP_HEIGHT;
        
        /* Name */
        STRPTR syncname = HIDDNouveauAlloc(32);
        sprintf(syncname, "NV:%dx%d@%d", mode->hdisplay, mode->vdisplay, mode->vrefresh);
        
        sync[j].ti_Tag = aHidd_Sync_Description;   sync[j++].ti_Data = (IPTR)syncname;
        
        sync[j].ti_Tag = TAG_DONE;                 sync[j++].ti_Data = 0UL;
        
        syncs[i].ti_Tag = aHidd_Gfx_SyncTags;
        syncs[i].ti_Data = (IPTR)sync;
    }
    
    return syncs;
}

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

    LOCK_ENGINE
    LOCK_BITMAP
    
    /* Check if passed bitmap has been registered as framebuffer */
    if (bmdata->fbid == 0)
    {
        UNLOCK_BITMAP
        UNLOCK_ENGINE
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
    struct nouveau_device_priv *nvdev = nouveau_device(carddata->dev);
    LONG i;
    drmModeConnectorPtr selectedconnector = NULL;
    HIDDT_ModeID modeid;
    OOP_Object * sync;
    OOP_Object * pf;
    IPTR pixel, e;
    IPTR hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;
    LONG ret;

    LOCK_ENGINE

    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);
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
        UNLOCK_ENGINE
        return FALSE;
    }

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
OOP_Object * METHOD(Nouveau, Root, New)
{
    drmModeCrtcPtr selectedcrtc = NULL;
    drmModeConnectorPtr selectedconnector = NULL;
    struct nouveau_device * dev = NULL;
    struct nouveau_device_priv * nvdev = NULL;
    struct TagItem * syncs = NULL;
    struct CardData * carddata = &(SD(cl)->carddata);
    LONG ret;
    ULONG selectedcrtcid;

    if (nouveau_init() < 0)
        return NULL;

    LOCK_ENGINE

    nouveau_device_open(&dev, "");
    nvdev = nouveau_device(dev);

    /* Select crtc and connector */
    if (!HIDDNouveauSelectConnectorCrtc(nvdev->fd, &selectedconnector, &selectedcrtc))
    {
        D(bug("[Nouveau] Not able to select connector and crtc\n"));

        UNLOCK_ENGINE

        return NULL;
    }
    
    selectedcrtcid = selectedcrtc->crtc_id;
    drmModeFreeCrtc(selectedcrtc);

    /* Read connector and build sync tags */
    syncs = HIDDNouveauCreateSyncTagsFromConnector(cl, selectedconnector);
    if (syncs == NULL)
    {
        D(bug("[Nouveau] Not able to read any sync modes\n"));
        UNLOCK_ENGINE
        return NULL;
    }
    

    /* Call super contructor */
    {
        struct TagItem pftags_24bpp[] = {
        { aHidd_PixFmt_RedShift,	8	}, /* 0 */
        { aHidd_PixFmt_GreenShift,	16	}, /* 1 */
        { aHidd_PixFmt_BlueShift,  	24	}, /* 2 */
        { aHidd_PixFmt_AlphaShift,	0	}, /* 3 */
        { aHidd_PixFmt_RedMask,		0x00ff0000 }, /* 4 */
        { aHidd_PixFmt_GreenMask,	0x0000ff00 }, /* 5 */
        { aHidd_PixFmt_BlueMask,	0x000000ff }, /* 6 */
        { aHidd_PixFmt_AlphaMask,	0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,	vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,		24	}, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,	4	}, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,	24	}, /* 11 */
        { aHidd_PixFmt_StdPixFmt,	vHidd_StdPixFmt_BGR032 }, /* 12 Native */
        { aHidd_PixFmt_BitMapType,	vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
        };

        struct TagItem pftags_16bpp[] = {
        { aHidd_PixFmt_RedShift,	16	}, /* 0 */
        { aHidd_PixFmt_GreenShift,	21	}, /* 1 */
        { aHidd_PixFmt_BlueShift,  	27	}, /* 2 */
        { aHidd_PixFmt_AlphaShift,	0	}, /* 3 */
        { aHidd_PixFmt_RedMask,		0x0000f800 }, /* 4 */
        { aHidd_PixFmt_GreenMask,	0x000007e0 }, /* 5 */
        { aHidd_PixFmt_BlueMask,	0x0000001f }, /* 6 */
        { aHidd_PixFmt_AlphaMask,	0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,	vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,		16	}, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,	2	}, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,	16	}, /* 11 */
        { aHidd_PixFmt_StdPixFmt,	vHidd_StdPixFmt_RGB16_LE }, /* 12 */
        { aHidd_PixFmt_BitMapType,	vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
        };

        struct TagItem modetags[] = {
	    { aHidd_Gfx_PixFmtTags,	(IPTR)pftags_24bpp	},
	    { aHidd_Gfx_PixFmtTags,	(IPTR)pftags_16bpp	},
        { TAG_MORE, (IPTR)syncs },  /* FIXME: sync tags will leak */
	    { TAG_DONE, 0UL }
        };

        struct TagItem mytags[] = {
	    { aHidd_Gfx_ModeTags,	(IPTR)modetags	},
            { aHidd_Name            , (IPTR)"Nouveau"     },
            { aHidd_HardwareName    , (IPTR)"Nvidia Gfx Adaptor"   },
            { aHidd_ProducerName    , (IPTR)"Nvidia Corporation"  },
	    { TAG_MORE, (IPTR)msg->attrList }
        };

        struct pRoot_New mymsg;

        mymsg.mID = msg->mID;
        mymsg.attrList = mytags;

        msg = &mymsg;


        o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

        D(bug("[Nouveau] GFX New\n"));

        if (o)
        {
            struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, o);
            /* Pass local information to class */
            gfxdata->selectedcrtcid = selectedcrtcid;
            gfxdata->selectedmode = NULL;
            gfxdata->selectedconnector = selectedconnector;
            carddata->dev = dev;
            ULONG gartsize = 0;
            UQUAD value;
            
            /* Check chipset architecture */
            switch (carddata->dev->chipset & 0xf0) 
            {
            case 0x00:
                carddata->architecture = NV_ARCH_04;
                break;
            case 0x10:
                carddata->architecture = NV_ARCH_10;
                break;
            case 0x20:
                carddata->architecture = NV_ARCH_20;
                break;
            case 0x30:
                carddata->architecture = NV_ARCH_30;
                break;
            case 0x40:
            case 0x60:
                carddata->architecture = NV_ARCH_40;
                break;
            case 0x50:
            case 0x80:
            case 0x90:
            case 0xa0:
                carddata->architecture = NV_ARCH_50;
                break;
            case 0xc0:
                carddata->architecture = NV_ARCH_C0;
                break;
            default:
                /* TODO: report error, how to handle it? */
                UNLOCK_ENGINE
                return NULL;
            }
            
            nouveau_device_get_param(carddata->dev, NOUVEAU_GETPARAM_BUS_TYPE, &value);
            if (value == NV_PCIE)
                carddata->IsPCIE = TRUE;
            else
                carddata->IsPCIE = FALSE;

            /* Allocate dma channel */
            ret = nouveau_channel_alloc(carddata->dev, NvDmaFB, NvDmaTT, 
                24 * 1024, &carddata->chan);
            if (ret < 0)
            {
                /* TODO: Check ret, how to handle ? */
            }

            /* Initialize acceleration objects */
        
            ret = HIDDNouveauAccelCommonInit(carddata);
            if (ret < 0)
            {
                /* TODO: Check ret, how to handle ? */
            }

            /* Allocate buffer object for cursor */
            nouveau_bo_new(carddata->dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 
                0, 64 * 64 * 4, &gfxdata->cursor);
            /* TODO: Check return, hot to handle */
            
            /* Allocate GART scratch buffer */
            if (carddata->dev->vm_gart_size > GART_BUFFER_SIZE)
                gartsize = GART_BUFFER_SIZE;
            else
                /* always leave 512kb for other things like the fifos */
                gartsize = carddata->dev->vm_gart_size - 512 * 1024;

            /* This can fail */
            nouveau_bo_new(carddata->dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP,
                0, gartsize, &carddata->GART);
            InitSemaphore(&carddata->gartsemaphore);
            
            /* Set initial pattern (else 16-bit ROPs are not working) */
            switch(carddata->architecture)
            {
            case(NV_ARCH_03):
            case(NV_ARCH_04):
            case(NV_ARCH_10):
            case(NV_ARCH_20):
            case(NV_ARCH_30):
            case(NV_ARCH_40):
                HIDDNouveauNV04SetPattern(carddata, ~0, ~0, ~0, ~0);
                break;
            case(NV_ARCH_50):
                HIDDNouveauNV50SetPattern(carddata, ~0, ~0, ~0, ~0);
                break;
            case(NV_ARCH_C0):
                HIDDNouveauNVC0SetPattern(carddata, ~0, ~0, ~0, ~0);
                break;
            }

            /* Create compositor object */
            {
                struct TagItem comptags [] =
                {
                    { aHidd_Compositor_GfxHidd, (IPTR)o },
                    { TAG_DONE, TAG_DONE }
                };
                gfxdata->compositor = OOP_NewObject(SD(cl)->compositorclass, NULL, comptags);
                /* TODO: Check if object was created, how to handle ? */
            }

        }
        UNLOCK_ENGINE

        return o;
    }
    UNLOCK_ENGINE

    return NULL;
}

/* FIXME: IMPLEMENT DISPOSE - calling nouveau_close(), freeing cursor bo, gart bo, 
    selectedconnector, gfxdata->compositor, HIDDNouveauAccelFree */

/* FIXME: IMPLEMENT DISPOSE BITMAP - REMOVE FROM FB IF MARKED AS SUCH */

OOP_Object * METHOD(Nouveau, Hidd_Gfx, CreateObject)
{
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, o);
    OOP_Object      *object = NULL;

    if (msg->cl == SD(cl)->basebm)
    {
        struct pHidd_Gfx_CreateObject mymsg;
        HIDDT_ModeID modeid;
        HIDDT_StdPixFmt stdpf;

        struct TagItem mytags [] =
        {
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_ClassPtr */
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_Align */
            { aHidd_BitMap_Nouveau_CompositorHidd, (IPTR)gfxdata->compositor },
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

VOID METHOD(Nouveau, Hidd_Gfx, CopyBox)
{
    OOP_Class * srcclass = OOP_OCLASS(msg->src);
    OOP_Class * destclass = OOP_OCLASS(msg->dest);
    
    if (IS_NOUVEAU_BM_CLASS(srcclass) && IS_NOUVEAU_BM_CLASS(destclass))
    {
        /* FIXME: add checks for pixel format, etc */
        struct HIDDNouveauBitMapData * srcdata = OOP_INST_DATA(srcclass, msg->src);
        struct HIDDNouveauBitMapData * destdata = OOP_INST_DATA(destclass, msg->dest);
        struct CardData * carddata = &(SD(cl)->carddata);
        BOOL ret = FALSE;
        
        D(bug("[Nouveau] CopyBox 0x%x -> 0x%x\n", msg->src, msg->dest));

        LOCK_ENGINE

        LOCK_MULTI_BITMAP
        LOCK_BITMAP_BM(srcdata)
        LOCK_BITMAP_BM(destdata)
        UNLOCK_MULTI_BITMAP
        UNMAP_BUFFER_BM(srcdata)
        UNMAP_BUFFER_BM(destdata)
        
        switch(carddata->architecture)
        {
        case(NV_ARCH_03):
        case(NV_ARCH_04):
        case(NV_ARCH_10):
        case(NV_ARCH_20):
        case(NV_ARCH_30):
        case(NV_ARCH_40):
            ret = HIDDNouveauNV04CopySameFormat(carddata, srcdata, destdata, 
                        msg->srcX, msg->srcY, msg->destX, msg->destY, 
                        msg->width, msg->height, GC_DRMD(msg->gc));
            break;
        case(NV_ARCH_50):
            ret = HIDDNouveauNV50CopySameFormat(carddata, srcdata, destdata, 
                        msg->srcX, msg->srcY, msg->destX, msg->destY, 
                        msg->width, msg->height, GC_DRMD(msg->gc));
            break;
        case(NV_ARCH_C0):
            ret = HIDDNouveauNVC0CopySameFormat(carddata, srcdata, destdata, 
                        msg->srcX, msg->srcY, msg->destX, msg->destY, 
                        msg->width, msg->height, GC_DRMD(msg->gc));
            break;
        }

        UNLOCK_BITMAP_BM(destdata);
        UNLOCK_BITMAP_BM(srcdata);

        UNLOCK_ENGINE

        if (ret)
            return;
        
        /* If operation failed, fallback to default method */
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    
}

VOID METHOD(Nouveau, Root, Get)
{
    ULONG idx;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gfx_NoFrameBuffer:
            *msg->storage = (IPTR)TRUE;
            return;
        case aoHidd_Gfx_SupportsHWCursor:
            *msg->storage = (IPTR)TRUE;
            return;
        case aoHidd_Gfx_HWSpriteTypes:
            *msg->storage = vHidd_SpriteType_DirectColor;
            return;
        case aoHidd_Gfx_DriverName:
            *msg->storage = (IPTR)"Nouveau";
            return;
        case aoHidd_Gfx_MemoryAttribs:
            {
                struct TagItem *matstate = (struct TagItem *)msg->storage;
                if (matstate)
                {
                    struct TagItem *matag;
                    while ((matag = NextTagItem(&matstate)))
                    {
                        switch(matag->ti_Tag)
                        {
                            case tHidd_Gfx_MemTotal:
                                {
                                    UQUAD value;
                                    nouveau_device_get_param(SD(cl)->carddata.dev, NOUVEAU_GETPARAM_VRAM_SIZE, &value);
                                    matag->ti_Data = (IPTR)value;
                                }
                                break;
                            case tHidd_Gfx_MemAddressableTotal:
                                {
                                    UQUAD value;
                                    nouveau_device_get_param(SD(cl)->carddata.dev, NOUVEAU_GETPARAM_GART_SIZE, &value);
                                    matag->ti_Data = (IPTR)value;
                                }
                                break;
                            case tHidd_Gfx_MemFree:
                                {
                                    UQUAD value;
                                    nouveau_device_get_param(SD(cl)->carddata.dev, NOUVEAU_GETPARAM_VRAM_FREE, &value);
                                    matag->ti_Data = (IPTR)value;
                                }
                                break;
                            case tHidd_Gfx_MemAddressableFree:
                                {
                                    UQUAD value;
                                    nouveau_device_get_param(SD(cl)->carddata.dev, NOUVEAU_GETPARAM_GART_FREE, &value);
                                    matag->ti_Data = (IPTR)value;
                                }
                                break;
                        }
                    }
                }
            }
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

ULONG METHOD(Nouveau, Hidd_Gfx, ShowViewPorts)
{
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, o);
    struct pHidd_Compositor_BitMapStackChanged bscmsg =
    {
        mID : OOP_GetMethodID(IID_Hidd_Compositor, moHidd_Compositor_BitMapStackChanged),
        data : msg->Data
    };
    
    D(bug("[Nouveau] ShowViewPorts enter TopLevelBM %x\n", (msg->Data ? (msg->Data->Bitmap) : NULL)));

    OOP_DoMethod(gfxdata->compositor, (OOP_Msg)&bscmsg);

    return TRUE; /* Indicate driver supports this method */
}

#if AROS_BIG_ENDIAN
#define Machine_ARGB32 vHidd_StdPixFmt_ARGB32
#else
#define Machine_ARGB32 vHidd_StdPixFmt_BGRA32
#endif

BOOL METHOD(Nouveau, Hidd_Gfx, SetCursorShape)
{
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, o);
        
    if (msg->shape == NULL)
    {
        /* Hide cursor */
        HIDDNouveauShowCursor(o, FALSE);
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
        nouveau_bo_map(gfxdata->cursor, NOUVEAU_BO_WR);

        /* Clear the matrix */
        for (i = 0; i < 64 * 64; i++)
            ((ULONG*)gfxdata->cursor->map)[i] = 0;

        /* Get data from the bitmap */
        HIDD_BM_GetImage(msg->shape, (UBYTE *)curimage, 64 * 4, 0, 0, 
            width, height, Machine_ARGB32);
        
        if (carddata->architecture < NV_ARCH_50)
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

        nouveau_bo_unmap(gfxdata->cursor);
        
        /* Show updated cursor */
        HIDDNouveauShowCursor(o, TRUE);

        UNLOCK_ENGINE
    }

    return TRUE;   
}

BOOL METHOD(Nouveau, Hidd_Gfx, SetCursorPos)
{
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device_priv * nvdev = nouveau_device(carddata->dev);

    LOCK_ENGINE
    drmModeMoveCursor(nvdev->fd, gfxdata->selectedcrtcid, msg->x, msg->y);
    UNLOCK_ENGINE

    return TRUE;
}

VOID METHOD(Nouveau, Hidd_Gfx, SetCursorVisible)
{
    HIDDNouveauShowCursor(o, msg->visible);
}

static struct HIDD_ModeProperties modeprops = 
{
    DIPF_IS_SPRITES,
    1,
    COMPF_ABOVE
};

ULONG METHOD(Nouveau, Hidd_Gfx, ModeProperties)
{
    ULONG len = msg->propsLen;

    if (len > sizeof(modeprops))
        len = sizeof(modeprops);
    CopyMem(&modeprops, msg->props, len);

    return len;
}



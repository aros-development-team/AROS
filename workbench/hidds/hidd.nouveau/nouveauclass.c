/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"
#include "nouveau/nouveau_class.h"

#include <proto/utility.h>

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>

#include "arosdrmmode.h"

/* Declaration of nouveau initialization function */
int nouveau_init(void);

#undef HiddPixFmtAttrBase
#undef HiddGfxAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define HiddPixFmtAttrBase  (SD(cl)->pixFmtAttrBase)
#define HiddGfxAttrBase     (SD(cl)->gfxAttrBase)
#define HiddSyncAttrBase    (SD(cl)->syncAttrBase)
#define HiddBitMapAttrBase  (SD(cl)->bitMapAttrBase)

/* HELPER FUNCTIONS */
static VOID HIDDNouveauShowCursor(OOP_Class * cl, OOP_Object * gfx, BOOL visible)
{
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, gfx);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device_priv * nvdev = nouveau_device(carddata->dev);

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
}

static BOOL HIDDNouveauSelectConnectorCrtc(LONG fd, drmModeConnectorPtr * selectedconnector,
    drmModeCrtcPtr * selectedcrtc)
{
    *selectedconnector = NULL;
    *selectedcrtc = NULL;
    drmModeResPtr drmmode = NULL;
    drmModeEncoderPtr selectedencoder = NULL;
    LONG i; ULONG crtc_id;

    /* Get all components information */
    drmmode = drmModeGetResources(fd);
    if (!drmmode)
    {
        D(bug("[Nouveau] Not able to get resources information\n"));
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
        return FALSE;
    }

    /* Selecting crtc (from encoder) */
    selectedencoder = drmModeGetEncoder(fd, (*selectedconnector)->encoder_id);
    
    if (!selectedencoder)
    {
        D(bug("[Nouveau] Not able to get encoder information for enc_id %d\n", (*selectedconnector)->encoder_id));
        drmModeFreeConnector(*selectedconnector);
        *selectedconnector = NULL;
        drmModeFreeResources(drmmode);
        return FALSE;
    }

    /* WARNING: CRTC_ID from encoder seems to be zero after first mode switch */
    crtc_id = selectedencoder->crtc_id;
    drmModeFreeEncoder(selectedencoder);

    *selectedcrtc = drmModeGetCrtc(fd, crtc_id);
    if (!(*selectedcrtc))
    {
        D(bug("[Nouveau] Not able to get crtc information for crtc_id %d\n", crtc_id));
        drmModeFreeConnector(*selectedconnector);
        *selectedconnector = NULL;
        drmModeFreeResources(drmmode);
        return FALSE;
    }
    
    drmModeFreeResources(drmmode);
    return TRUE;
}    

static BOOL HIDDNouveauSwitchToVideoMode(OOP_Class * cl, OOP_Object * gfx, OOP_Object * bm)
{
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(OOP_OCLASS(gfx), gfx);
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(OOP_OCLASS(bm), bm);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device_priv *nvdev = nouveau_device(carddata->dev);
    uint32_t output_ids[] = {gfxdata->selectedconnectorid};
    uint32_t output_count = 1;
    LONG i;
    drmModeConnectorPtr selectedconnector = NULL;
    drmModeModeInfoPtr  selectedmode = NULL;
    drmModeCrtcPtr      selectedcrtc = NULL;
    HIDDT_ModeID modeid;
    OOP_Object * sync;
    OOP_Object * pf;
    IPTR pixel;
    IPTR hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;
    LONG ret;

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

    selectedconnector = drmModeGetConnector(nvdev->fd, gfxdata->selectedconnectorid);
    if (!selectedconnector)
    {
        D(bug("[Nouveau] Failed to read connector %d information\n", gfxdata->selectedconnectorid));
        return FALSE;
    }
    
    selectedcrtc = drmModeGetCrtc(nvdev->fd, gfxdata->selectedcrtcid);
    if (!selectedcrtc)
    {
        drmModeFreeConnector(selectedconnector);
        D(bug("[Nouveau] Failed to read crtc %d information\n", gfxdata->selectedcrtcid));
        return FALSE;
    }

    D(bug("[Nouveau] Connector %d, CRTC %d\n", selectedconnector->connector_id, 
        selectedcrtc->crtc_id));

    

    /* Select mode */
    for (i = 0; i < selectedconnector->count_modes; i++)
    {
        drmModeModeInfoPtr mode = &selectedconnector->modes[i];
        
        /* FIXME: Maybe take clock into account? */
        if ((mode->hdisplay == hdisp) && (mode->vdisplay == vdisp) &&
            (mode->hsync_start == hstart) && (mode->vsync_start == vstart) &&
            (mode->hsync_end == hend) && (mode->vsync_end == vend))
        {
            selectedmode = mode;
            break;
        }
    }
    
    if (!selectedmode)
    {
        D(bug("[Nouveau] Not able to select mode\n"));
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
            drmModeFreeConnector(selectedconnector);
            drmModeFreeCrtc(selectedcrtc);
            D(bug("[Nouveau] Not able to add framebuffer\n"));
            return FALSE;
        }
    }


    /* Switch mode */
    ret = drmModeSetCrtc(nvdev->fd, selectedcrtc->crtc_id,
	        bmdata->fbid, selectedcrtc->x, selectedcrtc->y, output_ids, 
	        output_count, selectedmode);
    if (ret)
    {
        drmModeFreeConnector(selectedconnector);
        drmModeFreeCrtc(selectedcrtc);
        D(bug("[Nouveau] Not able to set crtc\n"));
        return FALSE;        
    }

    HIDDNouveauShowCursor(cl , gfx, TRUE);
        
    drmModeFreeConnector(selectedconnector);
    drmModeFreeCrtc(selectedcrtc);
    
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
    syncs = AllocVec(sizeof(struct TagItem) * modescount, MEMF_ANY);
    
    for (i = 0; i < modescount; i++)
    {
        struct TagItem * sync = AllocVec(sizeof(struct TagItem) * 11, MEMF_ANY);
        
        drmModeModeInfoPtr mode = &connector->modes[i];

        sync[0].ti_Tag = aHidd_Sync_PixelClock;     sync[0].ti_Data = mode->clock;
        sync[1].ti_Tag = aHidd_Sync_HDisp;          sync[1].ti_Data = mode->hdisplay;
        sync[2].ti_Tag = aHidd_Sync_HSyncStart;     sync[2].ti_Data = mode->hsync_start;
        sync[3].ti_Tag = aHidd_Sync_HSyncEnd;       sync[3].ti_Data = mode->hsync_end;
        sync[4].ti_Tag = aHidd_Sync_HTotal;         sync[4].ti_Data = mode->htotal;
        sync[5].ti_Tag = aHidd_Sync_VDisp;          sync[5].ti_Data = mode->vdisplay;
        sync[6].ti_Tag = aHidd_Sync_VSyncStart;     sync[6].ti_Data = mode->vsync_start;
        sync[7].ti_Tag = aHidd_Sync_VSyncEnd;       sync[7].ti_Data = mode->vsync_end;
        sync[8].ti_Tag = aHidd_Sync_VTotal;         sync[8].ti_Data = mode->vtotal;
        
        /* Name */
        STRPTR syncname = AllocVec(32, MEMF_ANY | MEMF_CLEAR);
        sprintf(syncname, "NV:%dx%d@%d", mode->hdisplay, mode->vdisplay, mode->vrefresh);
        
        sync[9].ti_Tag = aHidd_Sync_Description;    sync[9].ti_Data = (IPTR)syncname;
        
        sync[10].ti_Tag = TAG_DONE;                 sync[10].ti_Data = 0UL;
        
        syncs[i].ti_Tag = aHidd_Gfx_SyncTags;
        syncs[i].ti_Data = (IPTR)sync;
    }
    
    return syncs;
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
    ULONG selectedconnectorid;
    ULONG selectedcrtcid;
    
    nouveau_init();

    nouveau_device_open(&dev, "");
    nvdev = nouveau_device(dev);

    /* Select crtc and connector */
    if (!HIDDNouveauSelectConnectorCrtc(nvdev->fd, &selectedconnector, &selectedcrtc))
    {
        D(bug("[Nouveau] Not able to select connector and crtc\n"));
        return NULL;
    }
    
    selectedconnectorid = selectedconnector->connector_id;
    selectedcrtcid = selectedcrtc->crtc_id;
        
    drmModeFreeCrtc(selectedcrtc);

    /* Read connector and build sync tags */
    syncs = HIDDNouveauCreateSyncTagsFromConnector(cl, selectedconnector);
    drmModeFreeConnector(selectedconnector);
    if (syncs == NULL)
    {
        D(bug("[Nouveau] Not able to read any sync modes\n"));
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
	    { TAG_MORE, (IPTR)msg->attrList }
        };

        struct pRoot_New mymsg;

        mymsg.mID = msg->mID;
        mymsg.attrList = mytags;

        msg = &mymsg;


        o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

        D(bug("Nouveau::New\n"));

        if (o)
        {
            struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, o);
            /* Pass local information to class */
            gfxdata->selectedconnectorid = selectedconnectorid;
            gfxdata->selectedcrtcid = selectedcrtcid;
            carddata->dev = dev;
            
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
            default:
                /* TODO: report error, how to handle it? */
                return NULL;
            }

            /* Allocate dma channel */
            ret = nouveau_channel_alloc(carddata->dev, NvDmaFB, NvDmaTT, &carddata->chan);
            /* TODO: Check ret, how to handle ? */

            /* Initialize acceleration objects */
            ret = NVAccelCommonInit(carddata);
            /* TODO: Check ret, how to handle ? */

            /* Allocate buffer object for cursor */
            nouveau_bo_new(carddata->dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 
                0, 64 * 64 * 4, &gfxdata->cursor);
            /* TODO: Check return, hot to handle */
        }

        return o;
    }
    
    return NULL;
}

/* FIXME: IMPLEMENT DISPOSE - calling nouveau_close() */

/* FIXME: IMPLEMENT DISPOSE BITMAP - REMOVE FROM FB IF MARKED AS SUCH */

OOP_Object * METHOD(Nouveau, Hidd_Gfx, NewBitMap)
{
    BOOL displayable, framebuffer;
    OOP_Class *classptr = NULL;
    struct TagItem mytags[2];
    struct pHidd_Gfx_NewBitMap mymsg;

    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

    D(bug("[Nouveau] NewBitmap: framebuffer=%d, displayable=%d\n", framebuffer, displayable));

    /* FIXME: The framebuffer bitmap for NV50 is created in a different way than
    all other bitmaps */

    if (framebuffer)
    {
        /* If the user asks for a framebuffer map we must ALLWAYS supply a class */
        classptr = SD(cl)->bmclass;
    }
    else if (displayable)
    {
        classptr = SD(cl)->bmclass;
    }
    else
    {
        HIDDT_ModeID modeid;
        /*
        For the non-displayable case we can either supply a class ourselves
        if we can optimize a certain type of non-displayable bitmaps. Or we
        can let the superclass create on for us.

        The attributes that might come from the user deciding the bitmap
        pixel format are:
        - aHidd_BitMap_ModeID:	a modeid. create a nondisplayable
        bitmap with the size  and pixelformat of a gfxmode.
        - aHidd_BitMap_StdPixFmt: a standard pixelformat as described in
        hidd/graphics.h
        - aHidd_BitMap_Friend: if this is supplied and none of the two above
        are supplied, then the pixel format of the created bitmap
        will be the same as the one of the friend bitmap.

        These tags are listed in prioritized order, so if
        the user supplied a ModeID tag, then you should not care about StdPixFmt
        or Friend. If there is no ModeID, but a StdPixFmt tag supplied,
        then you should not care about Friend because you have to
        create the correct pixelformat. And as said above, if only Friend
        is supplied, you can create a bitmap with same pixelformat as Friend
        */


        modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        if (vHidd_ModeID_Invalid != modeid) 
        {
            /* User supplied a valid modeid. We can use our bitmap class */
            classptr = SD(cl)->bmclass;
        } 
        else 
        {
            /* We may create an offscreen bitmap if the user supplied a friend
            bitmap. But we need to check that he did not supplied a StdPixFmt
            */
            HIDDT_StdPixFmt stdpf;
            stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
            if (vHidd_StdPixFmt_Plane == stdpf) 
            {
                classptr = SD(cl)->planarbmclass;
            }
            else if (vHidd_StdPixFmt_Unknown == stdpf) 
            {
                /* No std pixfmt supplied */
                OOP_Object *friend;

                /* Did the user supply a friend bitmap ? */
                friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
                if (NULL != friend) 
                {
                    OOP_Class *friend_class = NULL;
                    /* User supplied friend bitmap. Is the friend bitmap a
                    NVidia Gfx hidd bitmap ? */
                    OOP_GetAttr(friend, aHidd_BitMap_ClassPtr, (APTR)&friend_class);

                    if (friend_class == SD(cl)->bmclass) 
                    {
                        /* Friend was NVidia hidd bitmap. Now we can supply our own class */
                        classptr = SD(cl)->bmclass;
                    }
                }
            }
        }
    }

    /* Do we supply our own class ? */
    if (NULL != classptr) 
    {
        /* Yes. We must let the superclass not that we do this. This is
        done through adding a tag in the frot of the taglist */
        mytags[0].ti_Tag	= aHidd_BitMap_ClassPtr;
        mytags[0].ti_Data	= (IPTR)classptr;
        mytags[1].ti_Tag	= TAG_MORE;
        mytags[1].ti_Data	= (IPTR)msg->attrList;

        /* Like in Gfx::New() we init a new message struct */
        mymsg.mID	= msg->mID;
        mymsg.attrList	= mytags;

        /* Pass the new message to the superclass */
        msg = &mymsg;
    }

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

#define IS_NOUVEAU_CLASS(x) (x == SD(cl)->bmclass)

VOID METHOD(Nouveau, Hidd_Gfx, CopyBox)
{
    OOP_Class * srcclass = OOP_OCLASS(msg->src);
    OOP_Class * destclass = OOP_OCLASS(msg->dest);
    
    if (IS_NOUVEAU_CLASS(srcclass) && IS_NOUVEAU_CLASS(destclass))
    {
        /* FIXME: add checks for pixel format, etc */
        struct HIDDNouveauBitMapData * srcdata = OOP_INST_DATA(srcclass, msg->src);
        struct HIDDNouveauBitMapData * destdata = OOP_INST_DATA(destclass, msg->dest);
        struct CardData * carddata = &(SD(cl)->carddata);
        BOOL ret = FALSE;
        
        LOCK_MULTI_BITMAP;
        LOCK_BITMAP_BM(srcdata);
        LOCK_BITMAP_BM(destdata);
        UNLOCK_MULTI_BITMAP;
        
        if (carddata->architecture < NV_ARCH_50)
        {
            ret = HIDDNouveauNV04CopySameFormat(carddata, srcdata, destdata, 
                        msg->srcX, msg->srcY, msg->destX, msg->destY, 
                        msg->width, msg->height, GC_DRMD(msg->gc));
        }
        else
        {
            ret = HIDDNouveauNV50CopySameFormat(carddata, srcdata, destdata, 
                        msg->srcX, msg->srcY, msg->destX, msg->destY, 
                        msg->width, msg->height, GC_DRMD(msg->gc));
        }

        UNLOCK_BITMAP_BM(destdata);
        UNLOCK_BITMAP_BM(srcdata);

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
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object * METHOD(Nouveau, Hidd_Gfx, Show)
{
    D(bug("[Nouveau] Show enter BM %x\n", msg->bitMap));

    if (msg->bitMap)
    {
        OOP_Class * bmclass = OOP_OCLASS(msg->bitMap);
        
        if (IS_NOUVEAU_CLASS(bmclass))
        {
            if (!HIDDNouveauSwitchToVideoMode(cl, o, msg->bitMap))
            {
                bug("[Nouveau] Video mode not set\n");
                return NULL;
            }
            else
            {
                /* TEMP - FIXME HACK FOR GALLIUM */
                SD(cl)->screenbitmap = (struct HIDDNouveauBitMapData *)OOP_INST_DATA(bmclass, msg->bitMap);
                /* TEMP - FIXME HACK FOR GALLIUM */
            }
        }
    }

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
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
        HIDDNouveauShowCursor(cl , o, FALSE);
    }
    else
    {
        IPTR width, height;
        ULONG i;
        ULONG x, y;
        ULONG curimage[64 * 64];
        
        OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
        OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);


        if (width > 64) width = 64;
        if (height > 64) height = 64;

        /* Map the cursor buffer */
        nouveau_bo_map(gfxdata->cursor, NOUVEAU_BO_WR);

        /* Clear the matrix */
        for (i = 0; i < 64 * 64; i++)
            ((ULONG*)gfxdata->cursor->map)[i] = 0;

        /* Get data from the bitmap */
        HIDD_BM_GetImage(msg->shape, (UBYTE *)curimage, 64 * 4, 0, 0, 
            width, height, Machine_ARGB32);

        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
            {
                ULONG offset = y * 64 + x;
                writel(curimage[offset], ((ULONG *)gfxdata->cursor->map) + (offset));
            }

        nouveau_bo_unmap(gfxdata->cursor);
    }

    return TRUE;   
}

VOID METHOD(Nouveau, Hidd_Gfx, SetCursorPos)
{
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device_priv * nvdev = nouveau_device(carddata->dev);

    drmModeMoveCursor(nvdev->fd, gfxdata->selectedcrtcid, msg->x, msg->y);
}

VOID METHOD(Nouveau, Hidd_Gfx, SetCursorVisible)
{
    HIDDNouveauShowCursor(cl, o, msg->visible);
}



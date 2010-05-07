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

/* HACK HACK HACK HACK */



#include "arosdrmmode.h"

struct nouveau_device * hackdev = NULL;

int nouveau_init(void);

static void init_nouveau_and_set_video_mode()
{
    bug("Before init\n");
    nouveau_init();
    

    nouveau_device_open(&hackdev, "");


}

/* HACK HACK HACK HACK */ 


#undef HiddPixFmtAttrBase
#undef HiddGfxAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define HiddPixFmtAttrBase  (SD(cl)->pixFmtAttrBase)
#define HiddGfxAttrBase     (SD(cl)->gfxAttrBase)
#define HiddSyncAttrBase    (SD(cl)->syncAttrBase)
#define HiddBitMapAttrBase  (SD(cl)->bitMapAttrBase)

/* HELPER FUNCTIONS */
static BOOL HIDDNouveauNV04CopySameFormat(struct HIDDNouveauData * gfxdata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    ULONG srcX, ULONG srcY, ULONG destX, ULONG destY, ULONG width, ULONG height)
{
    struct nouveau_channel * chan = gfxdata->chan;
    struct nouveau_bo * src_bo = srcdata->bo;
    struct nouveau_bo * dst_bo = destdata->bo;
    struct nouveau_grobj *surf2d = gfxdata->NvContextSurfaces;
    struct nouveau_grobj *blit = gfxdata->NvImageBlit;
    LONG fmt;

    if (srcdata->bytesperpixel != destdata->bytesperpixel)
        return FALSE;

    if (!NVAccelGetCtxSurf2DFormatFromPixmap(destdata, &fmt))
        return FALSE;
    
    /* Prepare copy */
    if (MARK_RING(chan, 64, 2))
        return FALSE;

    /* TODO: Understand what planemask is used for */
//  	planemask |= ~0 << pDstPixmap->drawable.bitsPerPixel;
//	if (planemask != ~0 || alu != GXcopy) {
//		if (pDstPixmap->drawable.bitsPerPixel == 32) {
//			MARK_UNDO(chan);
//			return FALSE;
//		}

//		BEGIN_RING(chan, blit, NV04_IMAGE_BLIT_SURFACE, 1);
//		OUT_RING  (chan, pNv->NvContextSurfaces->handle);
//		BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_OPERATION, 1);
//		OUT_RING  (chan, 1); /* ROP_AND */

//		NV04EXASetROP(pScrn, alu, planemask);
//	} else {
        BEGIN_RING(chan, blit, NV04_IMAGE_BLIT_SURFACE, 1);
        OUT_RING  (chan, gfxdata->NvContextSurfaces->handle);
        BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_OPERATION, 1);
        OUT_RING  (chan, 3); /* SRCCOPY */
//	}

    BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
    OUT_RING  (chan, fmt);
    OUT_RING  (chan, destdata->pitch << 16 | srcdata->pitch );
    if (OUT_RELOCl(chan, src_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD) ||
        OUT_RELOCl(chan, dst_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
        MARK_UNDO(chan);
        return FALSE;
    }

    /* TODO: State resubmit preparation. What does it do? */

    /* Execute copy */
    WAIT_RING (chan, 4);
    BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_POINT_IN, 3);
    OUT_RING  (chan, (srcY << 16) | srcX);
    OUT_RING  (chan, (destY << 16) | destX);
    OUT_RING  (chan, (height  << 16) | width);

    /* FIXME: !!!!!VERY WRONG - SOMEONE CAN READ/WRITE AT THE SAME TIME USING MAP */
    nouveau_bo_unmap(src_bo);
    nouveau_bo_unmap(dst_bo);

    /* FIXME: Make sure operations are done on unmapped bo */
    FIRE_RING (chan);

    /* FIXME: !!!!!VERY WRONG - SOMEONE CAN READ/WRITE AT THE SAME TIME USING MAP */
    nouveau_bo_map(src_bo, NOUVEAU_BO_RDWR);
    nouveau_bo_map(dst_bo, NOUVEAU_BO_RDWR);

    return TRUE;
}

static VOID HIDDNouveauSwitchToVideoMode(OOP_Class * cl, OOP_Object * gfx, OOP_Object * bm)
{
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(OOP_OCLASS(gfx), gfx);
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(OOP_OCLASS(bm), bm);
    struct nouveau_device_priv *nvdev = nouveau_device(gfxdata->dev);
    uint32_t output_ids[] = {0};
    uint32_t output_count = 1;
    LONG i;
    uint32_t fb_id;
    drmModeConnectorPtr selectedconnector = NULL;
    drmModeModeInfoPtr  selectedmode = NULL;
    drmModeCrtcPtr      selectedcrtc = NULL;
    HIDDT_ModeID modeid;
    OOP_Object * sync;
    OOP_Object * pf;
    IPTR pixel;
    IPTR hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;

    /* We should be able to get modeID from the bitmap */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);

    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[nouveau] Invalid ModeID\n"));
        return; /* FIXME: Report some kind of error */
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
    
    D(bug("[nouveau] Sync: %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
    pixel, hdisp, hstart, hend, htotal, vdisp, vstart, vend, vtotal));



    /* Get all components information */
    drmModeResPtr drmmode = drmModeGetResources(nvdev->fd);
    
    /* Get information about selected crtc */
    selectedcrtc = drmModeGetCrtc(nvdev->fd, drmmode->crtcs[0]);
    
    if (!selectedcrtc)
    {
        D(bug("[nouveau] Not able to get crtc information\n"));
        return; /* FIXME: Report some error */
    }
    
    /* Selecting connector */
    for (i = 0; i < drmmode->count_connectors; i++)
    {
        drmModeConnectorPtr connector = drmModeGetConnector(nvdev->fd, drmmode->connectors[i]);

        if (connector)
        {
            if (connector->connection == DRM_MODE_CONNECTED)
            {
                /* Found connected connector */
                selectedconnector = connector;
                break;
            }
            
            drmModeFreeConnector(connector);
        }
    }
    
    if (!selectedconnector)
    {
        D(bug("[nouveau] No connected connector\n"));
        return; /* FIXME: Report some error */
    }

    output_ids[0] = selectedconnector->connector_id;

    /* Select mode */
    for (i = 0; i < selectedconnector->count_modes; i++)
    {
        drmModeModeInfoPtr mode = &selectedconnector->modes[i];
        
        /* TODO: This selection seems naive - clock not taken into account */
        if ((mode->hdisplay == hdisp) && (mode->vdisplay == vdisp) &&
            (mode->hsync_start == hstart) && (mode->hsync_end == hend) &&
            (mode->vsync_start == vstart) && (mode->vsync_end == vend))
        {
            selectedmode = mode;
            break;
        }
    }
    
    if (!selectedmode)
    {
        D(bug("[nouveau] Not able to select mode\n"));
        return; /* FIXME: Report some error */
    }



    /* FIXME: For screen switching the bitmap might already one been a framebuffer 
       - needs to check for a ID somehow so that it is not added twice. Also the
       bitmap itself should not whether it is added as framebuffer so that it can
       unregister itself in Dispose */

    /* Add as frame buffer */
	drmModeAddFB(nvdev->fd, bmdata->width, bmdata->height, 
	        bmdata->depth, bmdata->bytesperpixel * 8, 
	        bmdata->pitch, bmdata->bo->handle, &fb_id);

    /* Switch mode */
    drmModeSetCrtc(nvdev->fd, selectedcrtc->crtc_id,
	        fb_id, selectedcrtc->x, selectedcrtc->y, output_ids, 
	        output_count, selectedmode);

    drmModeFreeConnector(selectedconnector);
    drmModeFreeCrtc(selectedcrtc);
    /* TODO: Free drmmode */
}



/* PUBLIC METHODS */
#define MAKE_SYNC(name,clock,hdisp,hstart,hend,htotal,vdisp,vstart,vend,vtotal,descr)	\
	struct TagItem sync_ ## name[]={			\
		{ aHidd_Sync_PixelClock,	clock*1000	},	\
		{ aHidd_Sync_HDisp,			hdisp 	},	\
		{ aHidd_Sync_HSyncStart,	hstart	},	\
		{ aHidd_Sync_HSyncEnd,		hend	},	\
		{ aHidd_Sync_HTotal,		htotal	},	\
		{ aHidd_Sync_VDisp,			vdisp	},	\
		{ aHidd_Sync_VSyncStart,	vstart	},	\
		{ aHidd_Sync_VSyncEnd,		vend	},	\
		{ aHidd_Sync_VTotal,		vtotal	},	\
		{ aHidd_Sync_Description,   	(IPTR)descr},	\
		{ TAG_DONE, 0UL }}

OOP_Object * METHOD(Nouveau, Root, New)
{
    bug("Nouveau::New\n");

    /* FIXME: Temporary - read real values from nouveau */
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
    
    MAKE_SYNC(1024x768_60, 65000,	//78654=60kHz, 75Hz. 65000=50kHz,62Hz
        1024, 1048, 1184, 1344,
         768,  771,  777,  806,
	 "Nouveau:1024x768");
	 
    MAKE_SYNC(640x480_60,   25175,
         640,  656,  752,  800,
         480,  489,  492,  525,
	 "Nouveau:640x480");

    MAKE_SYNC(800x600_56,	36000,	// 36000
         800,  824,  896, 1024,
         600,  601,  603,  625,
	 "Nouveau:800x600");
	 
    struct TagItem modetags[] = {
	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_24bpp	},
//	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_16bpp	},
//	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_15bpp	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_640x480_60	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_800x600_56	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_1024x768_60	},
//	{ aHidd_Gfx_SyncTags,	(IPTR)sync_1152x864_60  },
//	{ aHidd_Gfx_SyncTags,	(IPTR)sync_1280x1024_60 },
//	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1400x1050_60 },
//	{ aHidd_Gfx_SyncTags,	(IPTR)sync_1600x1200_60 },
//	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1280x800_60 },
//	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1440x900_60 },
//	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1680x1050_60 },
//	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1920x1080_60 },
//	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1920x1200_60 },

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

    if (o)
    {
        struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, o);
        LONG ret;
        
        /* HACK */
        init_nouveau_and_set_video_mode();
        /* HACK */

        /* Acquire device object FIXME: should be done directly via nouvea_open_device */
        gfxdata->dev = hackdev;

        /* Check chipset architecture */
        switch (gfxdata->dev->chipset & 0xf0) 
        {
        case 0x00:
            gfxdata->architecture = NV_ARCH_04;
            break;
        case 0x10:
            gfxdata->architecture = NV_ARCH_10;
            break;
        case 0x20:
            gfxdata->architecture = NV_ARCH_20;
            break;
        case 0x30:
            gfxdata->architecture = NV_ARCH_30;
            break;
        case 0x40:
        case 0x60:
            gfxdata->architecture = NV_ARCH_40;
            break;
        case 0x50:
        case 0x80:
        case 0x90:
        case 0xa0:
            gfxdata->architecture = NV_ARCH_50;
            break;
        default:
            /* TODO: report error, how to handle it? */
            return NULL;
        }

        /* Allocate dma channel */
        ret = nouveau_channel_alloc(gfxdata->dev, NvDmaFB, NvDmaTT, &gfxdata->chan);
        /* TODO: Check ret, how to handle ? */

        /* Initialize acceleration objects */
        ret = NVAccelCommonInit(gfxdata);
        /* TODO: Check ret, how to handle ? */

    }

    return o;
}

/* FIXME: IMPLEMENT DISPOSE */

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

    return (OOP_Object*)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
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
        struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, o);
        
        if (gfxdata->architecture < NV_ARCH_50)
        {
            BOOL ret = HIDDNouveauNV04CopySameFormat(gfxdata, srcdata, destdata, 
                        msg->srcX, msg->srcY, msg->destX, msg->destY, 
                        msg->width, msg->height);
            if (ret)
                return;
            /* If operation failed, fallback to default method */
        }
        else
        {
            /* TODO: Implement copying for NV50. Fall through for now */
        }
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
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object * METHOD(Nouveau, Hidd_Gfx, Show)
{
    if (msg->bitMap)
    {
        OOP_Class * bmclass = OOP_OCLASS(msg->bitMap);
        
        if (IS_NOUVEAU_CLASS(bmclass))
        {
            HIDDNouveauSwitchToVideoMode(cl, o, msg->bitMap);
        }
    }

    /* DO NOT CALL superclass Show - it relies on framebuffer */
    return msg->bitMap;
}

BOOL METHOD(Nouveau, Hidd_Gfx, SetCursorShape)
{
    bug("Nouveau::SetCursorShape\n");
    return FALSE;
}

VOID METHOD(Nouveau, Hidd_Gfx, SetCursorPos)
{
    bug("Nouveau::SetCursorPos\n");
}

VOID METHOD(Nouveau, Hidd_Gfx, SetCursorVisible)
{
    bug("Nouveau::SetCursorVisible\n");
}



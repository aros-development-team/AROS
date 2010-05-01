/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"

#include <proto/utility.h>

#include <aros/debug.h>
#include <proto/oop.h>


/* HACK HACK HACK HACK */

#include "nouveau/nouveau_drmif.h"
#include "nouveau/nouveau_bo.h"
#include "arosdrmmode.h"

APTR fbptr = NULL;

int nouveau_init(void);

static void init_nouveau_and_set_video_mode()
{
    bug("Before init\n");
    nouveau_init();
    
    struct nouveau_device *dev = NULL;
    uint32_t fb_id;
    nouveau_device_open(&dev, "");
    struct nouveau_device_priv *nvdev = nouveau_device(dev);

    /* create buffer */
  	struct nouveau_bo * fbo = NULL;
	nouveau_bo_new_tile(dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP,
				  0, 1024 * 4 * 768, 0, 0,
				  &fbo);
	nouveau_bo_map(fbo, NOUVEAU_BO_RDWR);
	fbptr = fbo->map;
    
    /* add as frame buffer */
	drmModeAddFB(nvdev->fd, 1024, 768, 24,
			   32, 1024 * 4, fbo->handle,
			   &fb_id);
			   
    bug("Added as framebuffer\n");			   
    uint32_t output_ids[] = {10};
    uint32_t output_count = 1;
    
    drmModeModeInfo mode =
    {
    .clock = 65000,
	.hdisplay = 1024,
	.hsync_start = 1048,
	.hsync_end = 1184, 
	.htotal = 1344, 
	.hskew = 0,
	.vdisplay = 768, 
	.vsync_start = 771, 
	.vsync_end=777, 
	.vtotal=806, 
	.vscan=0,

    .flags=   (1<<1) | (1<<3)
    };
    
    bug("Before switch mode\n");
    /* switch mode */
 	drmModeSetCrtc(nvdev->fd, 5 /*drmmode_crtc->mode_crtc->crtc_id*/,
			     fb_id, 0, 0, output_ids, output_count, &mode /*&kmode*/);

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

/* PUBLIC METHODS */
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
	 
    struct TagItem modetags[] = {
	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_24bpp	},
//	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_16bpp	},
//	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_15bpp	},
//	{ aHidd_Gfx_SyncTags,	(IPTR)sync_640x480_60	},
//	{ aHidd_Gfx_SyncTags,	(IPTR)sync_800x600_56	},
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



    
    
    init_nouveau_and_set_video_mode();





    mymsg.mID = msg->mID;
    mymsg.attrList = mytags;

    msg = &mymsg;
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return o;
}

OOP_Object * METHOD(Nouveau, Hidd_Gfx, Show)
{
    bug("Nouveau::Show\n");
    return NULL;
}

OOP_Object * METHOD(Nouveau, Hidd_Gfx, NewBitMap)
{
    BOOL displayable, framebuffer;
    OOP_Class *classptr = NULL;
    struct TagItem mytags[2];
    struct pHidd_Gfx_NewBitMap mymsg;

    bug("Nouveau::NewBitMap\n");

    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

    bug("[Nouveau] NewBitmap: framebuffer=%d, displayable=%d\n", framebuffer, displayable);

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
            /* User supplied a valid modeid. We can use our offscreen class */
            classptr = SD(cl)->offbmclass;
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
                        classptr = SD(cl)->offbmclass;
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



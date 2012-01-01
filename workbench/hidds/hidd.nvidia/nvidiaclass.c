/*
    Copyright � 2004-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NVidia gfx class
    Lang: English
*/

#include <exec/types.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "nv.h"
#include "nv_dma.h"

#define DEBUG 0
#include <aros/debug.h>

#define _sd (&((LIBBASETYPEPTR)cl->UserData)->sd)

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define HiddPCIDeviceAttrBase	(_sd->pciAttrBase)
#define HiddNVidiaBitMapAttrBase (_sd->nvBitMapAttrBase)
#define HiddBitMapAttrBase	(_sd->bitMapAttrBase)
#define HiddPixFmtAttrBase	(_sd->pixFmtAttrBase)
#define HiddGfxAttrBase		(_sd->gfxAttrBase)
#define HiddSyncAttrBase	(_sd->syncAttrBase)

/* Class methods */

VOID NV__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    BOOL found = FALSE;
    if (IS_GFX_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_Gfx_SupportsHWCursor:
		*msg->storage = (IPTR)TRUE;
		found = TRUE;
		break;

	    case aoHidd_Gfx_DPMSLevel:
		*msg->storage = _sd->dpms;
		found = TRUE;
		break;
        }
    }

    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return;
}

VOID NV__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    ULONG idx;

    struct TagItem *tag;
    struct TagItem *tags = msg->attrList;

    while ((tag = NextTagItem(&tags)))
    {
        if (IS_GFX_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
		case aoHidd_Gfx_DPMSLevel:
            LOCK_HW

		    DPMS(_sd, tag->ti_Data);
		    _sd->dpms = tag->ti_Data;

            UNLOCK_HW
		    break;
	    }
	}
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

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

OOP_Object *NV__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
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

    struct TagItem pftags_15bpp[] = {
	{ aHidd_PixFmt_RedShift,	17	}, /* 0 */
	{ aHidd_PixFmt_GreenShift,	22	}, /* 1 */
	{ aHidd_PixFmt_BlueShift,  	27	}, /* 2 */
	{ aHidd_PixFmt_AlphaShift,	0	}, /* 3 */
	{ aHidd_PixFmt_RedMask,		0x00007c00 }, /* 4 */
	{ aHidd_PixFmt_GreenMask,	0x000003e0 }, /* 5 */
	{ aHidd_PixFmt_BlueMask,	0x0000001f }, /* 6 */
	{ aHidd_PixFmt_AlphaMask,	0x00000000 }, /* 7 */
	{ aHidd_PixFmt_ColorModel,	vHidd_ColorModel_TrueColor }, /* 8 */
	{ aHidd_PixFmt_Depth,		15	}, /* 9 */
	{ aHidd_PixFmt_BytesPerPixel,	2	}, /* 10 */
	{ aHidd_PixFmt_BitsPerPixel,	15	}, /* 11 */
	{ aHidd_PixFmt_StdPixFmt,	vHidd_StdPixFmt_RGB15_LE }, /* 12 */
	{ aHidd_PixFmt_BitMapType,	vHidd_BitMapType_Chunky }, /* 15 */
	{ TAG_DONE, 0UL }
    };

    MAKE_SYNC(640x480_60,   25174,
         640,  656,  752,  800,
         480,  490,  492,  525,
	 "NVIDIA:640x480");

    MAKE_SYNC(800x600_56,	36000,	// 36000
         800,  824,  896, 1024,
         600,  601,  603,  625,
	 "NVIDIA:800x600");

    MAKE_SYNC(1024x768_60, 65000,	//78654=60kHz, 75Hz. 65000=50kHz,62Hz
        1024, 1048, 1184, 1344,
         768,  771,  777,  806,
	 "NVIDIA:1024x768");

    MAKE_SYNC(1152x864_60, 80000,
	1152, 1216, 1328, 1456,
	 864,  870,  875,  916,
	 "NVIDIA:1152x864");

    MAKE_SYNC(1280x1024_60, 107991,
	1280, 1328, 1440, 1688,
	1024, 1025, 1028, 1066,
	"NVIDIA:1280x1024");

    MAKE_SYNC(1400x1050_60, 121750,
	1400, 1488, 1632, 1864,
	1050, 1053, 1057, 1089,
	"NVIDIA:1400x1050");

    MAKE_SYNC(1600x1200_60, 155982,
	1600, 1632, 1792, 2048,
	1200, 1210, 1218, 1270,
	"NVIDIA:1600x1200");

    /* "new" 16:10 modes */

    MAKE_SYNC(1280x800_60, 83530,
	1280, 1344, 1480, 1680,
	800, 801, 804, 828,
	"NVIDIA:1280x800");

    MAKE_SYNC(1440x900_60, 106470,
	1440, 1520, 1672, 1904,
	900, 901, 904, 932,
	"NVIDIA:1440x900");

    MAKE_SYNC(1680x1050_60, 119000,
	1680, 1728, 1760, 1840,
	1050, 1053, 1059, 1080,
	"NVIDIA:1680x1050");
    
    MAKE_SYNC(1920x1080_60, 173000,
	1920, 2048, 2248, 2576,
	1080, 1083, 1088, 1120,
	"NVIDIA:1920x1080");

    MAKE_SYNC(1920x1200_60, 162090,
        1920, 1984, 2176, 2480,
        1200, 1201, 1204, 1250,
        "NVIDIA:1920x1200");

    struct TagItem modetags[] = {
	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_24bpp	},
	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_16bpp	},
	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_15bpp	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_640x480_60	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_800x600_56	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_1024x768_60	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_1152x864_60  },
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_1280x1024_60 },
	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1400x1050_60 },
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_1600x1200_60 },
	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1280x800_60 },
	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1440x900_60 },
	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1680x1050_60 },
	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1920x1080_60 },
	{ aHidd_Gfx_SyncTags,   (IPTR)sync_1920x1200_60 },

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

    EnterFunc(bug("[NVidia] nv::new()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	_sd->nvobject = o;
    }

    return o;
}

OOP_Object *NV__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o,
	    struct pHidd_Gfx_NewBitMap *msg)
{
    BOOL displayable, framebuffer;
    OOP_Class *classptr = NULL;
    struct TagItem mytags[2];
    struct pHidd_Gfx_NewBitMap mymsg;

    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

D(bug("[NVidia] NewBitmap: framebuffer=%d, displayable=%d\n", framebuffer, displayable));

    if (framebuffer)
    {
	/* If the user asks for a framebuffer map we must ALLWAYS supply a class */
	classptr = _sd->onbmclass;
    }
    else if (displayable)
    {
    	classptr = _sd->onbmclass;	//offbmclass;
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
	    is supplied, you can create a bitmap with same pixelformat as Frien
	*/


	modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
	if (vHidd_ModeID_Invalid != modeid) {
	    /* User supplied a valid modeid. We can use our offscreen class */
	    classptr = _sd->offbmclass;
	} else {
	    /* We may create an offscreen bitmap if the user supplied a friend
	       bitmap. But we need to check that he did not supplied a StdPixFmt
	    */
	    HIDDT_StdPixFmt stdpf;
	    stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
	    if (vHidd_StdPixFmt_Plane == stdpf) {
		classptr = _sd->planarbmclass;
	    }
	    else if (vHidd_StdPixFmt_Unknown == stdpf) {
		/* No std pixfmt supplied */
		OOP_Object *friend;

		/* Did the user supply a friend bitmap ? */
		friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
		if (NULL != friend) {
		    OOP_Class *friend_class = NULL;
		    /* User supplied friend bitmap. Is the friend bitmap a
		    NVidia Gfx hidd bitmap ? */
		    OOP_GetAttr(friend, aHidd_BitMap_ClassPtr, (APTR)&friend_class);
		    if (friend_class == _sd->onbmclass) {
			/* Friend was NVidia hidd bitmap. Now we can supply our own class */
			classptr = _sd->offbmclass;
		    }
		}
	    }
	}
    }

    D(bug("classptr = %p\n", classptr));
    /* Do we supply our own class ? */
    if (NULL != classptr) {
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

OOP_Object *NV__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o,
		        struct pHidd_Gfx_Show *msg)
{
    OOP_Object *fb = NULL;
    if (msg->bitMap)
    {
	nvBitMap *bm = OOP_INST_DATA(OOP_OCLASS(msg->bitMap), msg->bitMap);

	if (bm->state)
	{
	    /* Suppose bm has properly allocated state structure */
	    if (bm->fbgfx)
	    {
		bm->usecount++;

		LOCK_HW

		LoadState(_sd, bm->state);
		DPMS(_sd, _sd->dpms);

		fb = bm->BitMap;
		NVShowHideCursor(_sd, _sd->Card.cursorVisible);

		UNLOCK_HW
	    }
	}
    }

    if (!fb)
        fb = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return fb;
}

VOID NV__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    ULONG mode = GC_DRMD(msg->gc);
    IPTR src=0, dst=0;

    /* Check whether we can get Drawable attribute of our nVidia class */
    OOP_GetAttr(msg->src,   aHidd_nvBitMap_Drawable,	&src);
    OOP_GetAttr(msg->dest,  aHidd_nvBitMap_Drawable,	&dst);

    if (!dst || !src)
    {
	/* No. One of the bitmaps is not nVidia bitmap */
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
	/* Yes. Get the instance data of both bitmaps */
	nvBitMap *bm_src = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
	nvBitMap *bm_dst = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);

	/* Case -1: (To be fixed) one of the bitmaps have chunky outside GFX mem */
	if (!bm_src->fbgfx || !bm_dst->fbgfx)
	{
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	}
	/* Case 0: one of bitmaps is 8bpp, whereas the other is TrueColor one */
	else if ((bm_src->depth <= 8 || bm_dst->depth <= 8) &&
	    (bm_src->depth != bm_dst->depth))
	{
	    /* Unsupported case */
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    return;
	}
	/* Case 1: both bitmaps have the same depth - use Blit engine */
	else if (bm_src->depth == bm_dst->depth)
	{
	    LOCK_MULTI_BITMAP
	    LOCK_BITMAP_BM(bm_src)
	    LOCK_BITMAP_BM(bm_dst)
	    UNLOCK_MULTI_BITMAP

	    LOCK_HW

    	    _sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;
    	    _sd->gpu_busy = TRUE;

	    NVSetRopSolid(_sd, mode, ~0 << bm_src->depth);

	    if (bm_dst->surface_format != _sd->surface_format)
	    {
		NVDmaStart(&_sd->Card, SURFACE_FORMAT, 1);
		NVDmaNext(&_sd->Card, bm_dst->surface_format);
		_sd->surface_format = bm_dst->surface_format;
//		D(bug("[NVidia] surface_format <- %d\n", _sd->surface_format));
	    }
	    if ((bm_dst->pitch != _sd->dst_pitch) || (bm_src->pitch != _sd->src_pitch))
	    {
		NVDmaStart(&_sd->Card, SURFACE_PITCH, 1);
		NVDmaNext(&_sd->Card, (bm_dst->pitch << 16) | bm_src->pitch);
		_sd->src_pitch = bm_src->pitch;
		_sd->dst_pitch = bm_dst->pitch;
//		D(bug("[NVidia] pitch <- %08x\n", (_sd->dst_pitch << 16) | _sd->src_pitch));
	    }
	    if (bm_src->framebuffer != _sd->src_offset)
	    {
		NVDmaStart(&_sd->Card, SURFACE_OFFSET_SRC, 1);
		NVDmaNext(&_sd->Card, bm_src->framebuffer);
		_sd->src_offset = bm_src->framebuffer;
//		D(bug("[NVidia] src_offset=%p\n", _sd->src_offset));
	    }
	    if (bm_dst->framebuffer != _sd->dst_offset)
	    {
		NVDmaStart(&_sd->Card, SURFACE_OFFSET_DST, 1);
		NVDmaNext(&_sd->Card, bm_dst->framebuffer);
		_sd->dst_offset = bm_dst->framebuffer;
//		D(bug("[NVidia] dst_offset=%p\n", _sd->dst_offset));
	    }

	    NVDmaStart(&_sd->Card, BLIT_POINT_SRC, 3);
	    NVDmaNext(&_sd->Card, (msg->srcY << 16) | (msg->srcX & 0xffff));
	    NVDmaNext(&_sd->Card, (msg->destY << 16) | (msg->destX & 0xffff));
	    NVDmaNext(&_sd->Card, (msg->height << 16) | (msg->width & 0xffff));

	    NVDmaKickoff(&_sd->Card);
	    //NVSync(_sd);

	    UNLOCK_HW

	    UNLOCK_BITMAP_BM(bm_src)
	    UNLOCK_BITMAP_BM(bm_dst)

	}
	else /* Case 2: different bitmaps. use Stretch engine */
	{
	    LOCK_MULTI_BITMAP
	    LOCK_BITMAP_BM(bm_src)
	    LOCK_BITMAP_BM(bm_dst)
	    UNLOCK_MULTI_BITMAP

	    LOCK_HW

    	    _sd->Card.DMAKickoffCallback = NVDMAKickoffCallback;
    	    _sd->gpu_busy = TRUE;

	    if ((bm_dst->surface_format != _sd->surface_format) && bm_dst->depth != 15)
	    {

		NVDmaStart(&_sd->Card, SURFACE_FORMAT, 1);
		NVDmaNext(&_sd->Card, bm_dst->surface_format);
		_sd->surface_format = bm_dst->surface_format;
//		D(bug("[NVidia] surface_format <- %d\n", _sd->surface_format));
	    }

	    if (bm_dst->depth == 15)
	    {
		NVDmaStart(&_sd->Card, SURFACE_FORMAT, 1);
		NVDmaNext(&_sd->Card, SURFACE_FORMAT_DEPTH15);
		_sd->surface_format = SURFACE_FORMAT_DEPTH16;
	    }

	    if (bm_dst->pitch != _sd->dst_pitch)
	    {
		NVDmaStart(&_sd->Card, SURFACE_PITCH, 1);
		NVDmaNext(&_sd->Card, (bm_dst->pitch << 16) | _sd->src_pitch);
		_sd->dst_pitch = bm_dst->pitch;
//		D(bug("[NVidia] pitch <- %08x\n", (_sd->dst_pitch << 16) | _sd->src_pitch));
	    }

	    if (bm_dst->framebuffer != _sd->dst_offset)
	    {
		NVDmaStart(&_sd->Card, SURFACE_OFFSET_DST, 1);
		NVDmaNext(&_sd->Card, bm_dst->framebuffer);
		_sd->dst_offset = bm_dst->framebuffer;
//		D(bug("[NVidia] dst_offset=%p\n", _sd->dst_offset));
	    }

	    NVDmaStart(&_sd->Card, RECT_SOLID_COLOR, 1);
	    NVDmaNext(&_sd->Card, 0);

	    NVDmaStart(&_sd->Card, STRETCH_BLIT_FORMAT, 1);
	    switch (bm_src->depth)
	    {
		case 15:
		    NVDmaNext(&_sd->Card, STRETCH_BLIT_FORMAT_DEPTH15);
		    break;
		case 16:
		    NVDmaNext(&_sd->Card, STRETCH_BLIT_FORMAT_DEPTH16);
		    break;
		case 24:
		    NVDmaNext(&_sd->Card, STRETCH_BLIT_FORMAT_DEPTH24);
		    break;
		default:
		    NVDmaNext(&_sd->Card, STRETCH_BLIT_FORMAT_DEPTH8);
		    break;
	    }

	    NVDmaStart(&_sd->Card, STRETCH_BLIT_CLIP_POINT, 6);
	    NVDmaNext(&_sd->Card, 0x00000000);    // dst_CLip
	    NVDmaNext(&_sd->Card, 0xffffffff);    // dst_Clip
	    NVDmaNext(&_sd->Card, (msg->destY << 16) | (msg->destX));// dst_y | dst_x
	    NVDmaNext(&_sd->Card, (msg->height << 16)| (msg->width));// dst_h | dst_w
	    NVDmaNext(&_sd->Card, 1 << 20);  // src_w / dst_w 1:1
	    NVDmaNext(&_sd->Card, 1 << 20);  // src_h / dst_h 1:1

	    NVDmaStart(&_sd->Card, STRETCH_BLIT_SRC_SIZE, 4);
	    NVDmaNext(&_sd->Card, (msg->height << 16) | (msg->width));// src_h | src_w
	    NVDmaNext(&_sd->Card,
		(STRETCH_BLIT_SRC_FORMAT_FILTER_POINT_SAMPLE << 24) |   // BILINEAR | _POINT_SAMPLE
		(STRETCH_BLIT_SRC_FORMAT_ORIGIN_CORNER << 16) |
		(bm_src->pitch));				    // src_pitch
	    NVDmaNext(&_sd->Card, bm_src->framebuffer);		    // src_offset
	    NVDmaNext(&_sd->Card, ((msg->srcY << 20) & 0xffff0000)
		    | ((msg->srcX << 4) & 0xffff)); // src_y | src_x

	    NVDmaKickoff(&_sd->Card);

	    if (bm_dst->depth == 15)
	    {
		NVDmaStart(&_sd->Card, SURFACE_FORMAT, 1);
		NVDmaNext(&_sd->Card, SURFACE_FORMAT_DEPTH16);
	    }
	    //NVSync(_sd);

	    UNLOCK_HW

	    UNLOCK_BITMAP_BM(bm_src)
	    UNLOCK_BITMAP_BM(bm_dst)

	}

D(bug("[NVidia] CopyBox(src(%p,%d:%d@%d),dst(%p,%d:%d@%d),%d:%d\n",
		bm_src->framebuffer,msg->srcX,msg->srcY,bm_src->depth,
		bm_dst->framebuffer,msg->destX,msg->destY,bm_dst->depth,
		msg->width, msg->height));

	bm_src->usecount++;
	bm_dst->usecount++;
    }
}

#define ToRGB555(c) \
    (((c & 0xf80000) >> 9) | ((c & 0xf800) >> 6) | ((c & 0xf8) >> 3) | ((c & 0xFF000000) ? 0x8000 : 0))

/* Do we miss similar definition in include/hidd/graphics.h ? */
#if AROS_BIG_ENDIAN
#define Machine_ARGB32 vHidd_StdPixFmt_ARGB32
#else
#define Machine_ARGB32 vHidd_StdPixFmt_BGRA32
#endif

BOOL NV__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    if (msg->shape == NULL)
    {
	NVShowHideCursor(_sd, 0);
	_sd->Card.cursorVisible = 0;
	return TRUE;
    }
    else
    {
	IPTR width, height, x;

	OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
	OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);

	if (_sd->Card.alphaCursor)
	{
	    ULONG *curimg = (ULONG *)_sd->Card.CURSOR;
	    
	    if (width > 64) width = 64;
	    if (height > 64) height = 64;

	    LOCK_HW

	    /* Clear the matrix */
	    for (x = 0; x < 64*64; x++)
	        curimg[x] = 0;

	    /* Get data from the bitmap */
	    HIDD_BM_GetImage(msg->shape, (UBYTE *)curimg, 64*4, 0, 0, width, height, Machine_ARGB32);
	    
	    UNLOCK_HW
	    return TRUE;   
	}
	else
	{
	    ULONG *tmp;
	    UWORD *curimg = (UWORD *)_sd->Card.CURSOR;

	    if (width > 32) width = 32;
	    if (height > 32) height = 32;

	    /* Allocate a temporary buffer. It will be cleared because the pool was created with MEMF_CLEAR */
            tmp = AllocPooled(_sd->memPool, 4 * 32 * 32);
	    if (tmp) {
	        /* Get data from the bitmap, we need alpha channel too */
	        HIDD_BM_GetImage(msg->shape, (UBYTE *)tmp, 4 * 32, 0, 0, width, height, Machine_ARGB32);

	        LOCK_HW

		/* Now convert the data */
		for (x = 0; x < 32*32; x++)
		    curimg[x] = ToRGB555(tmp[x]);

		UNLOCK_HW

		FreePooled(_sd->memPool, tmp, 4 * 32 * 32);
		return TRUE;
	    } else
	        return FALSE;
	}
    }

}

VOID NV__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o,
		struct pHidd_Gfx_SetCursorVisible *msg)
{
    NVShowHideCursor(_sd, msg->visible);
    _sd->Card.cursorVisible = msg->visible;
}

VOID NV__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o,
	        struct pHidd_Gfx_SetCursorPos *msg)
{
    _sd->Card.PRAMDAC[0x0300 / 4] = (msg->y << 16) | (msg->x & 0xffff);
}
/* Class related functions */

#undef _sd
#define _sd sd

/*
    Allocates some memory area on GFX card, which may be sufficient for bitmap
    with given size and depth. The must_have bit may be defined but doesn't
    have to. If it is TRUE, the allocator will do everything to get the memory -
    eg. it will throw other bitmaps away from it or it will shift them within
    GFX memory
*/

IPTR AllocBitmapArea(struct staticdata *sd, ULONG width, ULONG height,
    ULONG bpp, BOOL must_have)
{
    IPTR result;

    LOCK_HW

    Forbid();
    result = (IPTR)Allocate(sd->CardMem, ((width * bpp + 63) & ~63) * height);
    Permit();

    D(bug("[NVidia] AllocBitmapArea(%dx%d@%d) = %p\n",
	width, height, bpp, result));
    /*
	If Allocate failed, make the 0xffffffff as return. If it succeeded, make
	the memory pointer relative to the begin of GFX memory
    */
    if (result == 0) --result;
    else result -= (IPTR)sd->Card.FrameBuffer;

    UNLOCK_HW

    /* Generic thing. Will be extended later */
    return result;
}

VOID FreeBitmapArea(struct staticdata *sd, IPTR bmp, ULONG width, ULONG height,
    ULONG bpp)
{
    APTR ptr = (APTR)(bmp + sd->Card.FrameBuffer);

    LOCK_HW

    D(bug("[NVidia] FreeBitmapArea(%p,%dx%d@%d)\n",
	bmp, width, height, bpp));

    Forbid();
    Deallocate(sd->CardMem, ptr, ((width * bpp + 63) & ~63) * height);
    Permit();

    UNLOCK_HW
}



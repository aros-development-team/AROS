/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
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

#define sd ((struct staticdata*)cl->UserData)

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define SysBase			(sd->sysbase)
#define OOPBase			(sd->oopbase)
#define UtilityBase		(sd->utilitybase)
#define HiddPCIDeviceAttrBase	(sd->pciAttrBase)
#define HiddNVidiaBitMapAttrBase (sd->nvBitMapAttrBase)
#define HiddBitMapAttrBase	(sd->bitMapAttrBase)
#define HiddPixFmtAttrBase	(sd->pixFmtAttrBase)
#define HiddGfxAttrBase		(sd->gfxAttrBase)
#define HiddSyncAttrBase	(sd->syncAttrBase)

/* Class methods */

static VOID nv__get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
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
		*msg->storage = sd->dpms;
		found = TRUE;
		break;
        }
    }

    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return;
}

static VOID nv__set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    ULONG idx;
    struct TagItem *tags, *tag;

    tags = msg->attrList;

    while ((tag = NextTagItem(&tags)))
    {
        if (IS_GFX_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
		case aoHidd_Gfx_DPMSLevel:
		    LOCK_HW
		    
		    DPMS(sd, tag->ti_Data);
		    sd->dpms = tag->ti_Data;
		    
		    UNLOCK_HW
		    break;
	    }
	}
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

#define MAKE_SYNC(name,clock,hdisp,hstart,hend,htotal,vdisp,vstart,vend,vtotal)	\
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
		{ TAG_DONE, 0UL }}

static OOP_Object *nv__new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
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
	{ aHidd_PixFmt_BitsPerPixel,	32	}, /* 11 */
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
	{ aHidd_PixFmt_BitsPerPixel,	16	}, /* 11 */
	{ aHidd_PixFmt_StdPixFmt,	vHidd_StdPixFmt_RGB15_LE }, /* 12 */
	{ aHidd_PixFmt_BitMapType,	vHidd_BitMapType_Chunky }, /* 15 */
	{ TAG_DONE, 0UL }
    };

    MAKE_SYNC(640x480_60,   25200,
         640,  656,  752,  800,
         480,  490,  492,  525);

    MAKE_SYNC(800x600_56,	36000,	// 36000
         800,  824,  896, 1024,
         600,  601,  603,  625);

    MAKE_SYNC(1024x768_60,	78654,
        1024, 1056, 1184, 1312,
         768,  772,  776,  792);

    struct TagItem modetags[] = {
	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_24bpp	},
	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_16bpp	},
	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags_15bpp	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_640x480_60	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_800x600_56	},
	{ aHidd_Gfx_SyncTags,	(IPTR)sync_1024x768_60	},
	
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
	sd->nvobject = o;
    }

    return o;
}

static OOP_Object *nv__newbitmap(OOP_Class *cl, OOP_Object *o, 
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
	classptr = sd->offbmclass;
    }
    else if (displayable)
    {
    	classptr = sd->onbmclass;	//offbmclass;
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
	    classptr = sd->offbmclass;
	} else {
	    /* We may create an offscreen bitmap if the user supplied a friend
	       bitmap. But we need to check that he did not supplied a StdPixFmt
	    */
	    HIDDT_StdPixFmt stdpf;
	    stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
	    if (vHidd_StdPixFmt_Unknown == stdpf) {
		/* No std pixfmt supplied */
		OOP_Object *friend;
	    
		/* Did the user supply a friend bitmap ? */
		friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
		if (NULL != friend) {
		    OOP_Object * gfxhidd;
		    /* User supplied friend bitmap. Is the friend bitmap a
		    NVidia Gfx hidd bitmap ? */
		    OOP_GetAttr(friend, aHidd_BitMap_GfxHidd, (APTR)&gfxhidd);
		    if (gfxhidd == o) {
			/* Friend was NVidia hidd bitmap. Now we can supply our own class */
			classptr = sd->offbmclass;		    
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

static OOP_Object *nv__show(OOP_Class *cl, OOP_Object *o, 
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
		
		LOCK_ALL
		LOCK_HW

		LoadState(sd, bm->state);
		DPMS(sd, sd->dpms);

		fb = bm->BitMap;
		NVShowHideCursor(sd, sd->Card.cursorVisible);
	    
		sd->Card.PRAMDAC[0x0300 / 4] = 0;

		UNLOCK_HW
		UNLOCK_ALL
	    }
	}
    }
    
    if (!fb)
        fb = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return fb;
}

static VOID nv__copybox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
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
	if ((bm_src->depth <= 8 || bm_dst->depth <= 8) &&
	    (bm_src->depth != bm_dst->depth))
	{
	    /* Unsupported case */
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    return;
	}
	/* Case 1: both bitmaps have the same depth - use Blit engine */
	if (bm_src->depth == bm_dst->depth)
	{
	    LOCK_ALL
	    LOCK_HW
	    
	    NVSetRopSolid(sd, mode, ~0);

	    if (bm_dst->surface_format != sd->surface_format)
	    {
		NVDmaStart(&sd->Card, SURFACE_FORMAT, 1);
		NVDmaNext(&sd->Card, bm_dst->surface_format);
		sd->surface_format = bm_dst->surface_format;
//		D(bug("[NVidia] surface_format <- %d\n", sd->surface_format));
	    }
	    if ((bm_dst->pitch != sd->dst_pitch) || (bm_src->pitch != sd->src_pitch))
	    {
		NVDmaStart(&sd->Card, SURFACE_PITCH, 1);
		NVDmaNext(&sd->Card, (bm_dst->pitch << 16) | bm_src->pitch);
		sd->src_pitch = bm_src->pitch;
		sd->dst_pitch = bm_dst->pitch;
//		D(bug("[NVidia] pitch <- %08x\n", (sd->dst_pitch << 16) | sd->src_pitch));
	    }
	    if (bm_src->framebuffer != sd->src_offset)
	    {
		NVDmaStart(&sd->Card, SURFACE_OFFSET_SRC, 1);
		NVDmaNext(&sd->Card, bm_src->framebuffer);
		sd->src_offset = bm_src->framebuffer;
//		D(bug("[NVidia] src_offset=%p\n", sd->src_offset));
	    }
	    if (bm_dst->framebuffer != sd->dst_offset)
	    {
		NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
		NVDmaNext(&sd->Card, bm_dst->framebuffer);
		sd->dst_offset = bm_dst->framebuffer;
//		D(bug("[NVidia] dst_offset=%p\n", sd->dst_offset));
	    }

	    NVDmaStart(&sd->Card, BLIT_POINT_SRC, 3);
	    NVDmaNext(&sd->Card, (msg->srcY << 16) | (msg->srcX & 0xffff));
	    NVDmaNext(&sd->Card, (msg->destY << 16) | (msg->destX & 0xffff));
	    NVDmaNext(&sd->Card, (msg->height << 16) | (msg->width & 0xffff));
	    
	    NVDmaKickoff(&sd->Card);
	    NVSync(sd);

	    UNLOCK_HW
	    UNLOCK_ALL
	}
	else /* Case 2: different bitmaps. use Stretch engine */
	{
	    LOCK_ALL
	    LOCK_HW

	    if ((bm_dst->surface_format != sd->surface_format) && bm_dst->depth != 15)
	    {
		
		NVDmaStart(&sd->Card, SURFACE_FORMAT, 1);
		NVDmaNext(&sd->Card, bm_dst->surface_format);
		sd->surface_format = bm_dst->surface_format;
//		D(bug("[NVidia] surface_format <- %d\n", sd->surface_format));
	    }

	    if (bm_dst->depth == 15)
	    {
		NVDmaStart(&sd->Card, SURFACE_FORMAT, 1);
		NVDmaNext(&sd->Card, SURFACE_FORMAT_DEPTH15);
		sd->surface_format = SURFACE_FORMAT_DEPTH16;
	    }

	    if (bm_dst->pitch != sd->dst_pitch)
	    {
		NVDmaStart(&sd->Card, SURFACE_PITCH, 1);
		NVDmaNext(&sd->Card, (bm_dst->pitch << 16) | sd->src_pitch);
		sd->dst_pitch = bm_dst->pitch;
//		D(bug("[NVidia] pitch <- %08x\n", (sd->dst_pitch << 16) | sd->src_pitch));
	    }

	    if (bm_dst->framebuffer != sd->dst_offset)
	    {
		NVDmaStart(&sd->Card, SURFACE_OFFSET_DST, 1);
		NVDmaNext(&sd->Card, bm_dst->framebuffer);
		sd->dst_offset = bm_dst->framebuffer;
//		D(bug("[NVidia] dst_offset=%p\n", sd->dst_offset));
	    }

	    NVDmaStart(&sd->Card, RECT_SOLID_COLOR, 1);
	    NVDmaNext(&sd->Card, 0);

	    NVDmaStart(&sd->Card, STRETCH_BLIT_FORMAT, 1);
	    switch (bm_src->depth)
	    {
		case 15:
		    NVDmaNext(&sd->Card, STRETCH_BLIT_FORMAT_DEPTH15);
		    break;
		case 16:
		    NVDmaNext(&sd->Card, STRETCH_BLIT_FORMAT_DEPTH16);
		    break;
		case 24:
		    NVDmaNext(&sd->Card, STRETCH_BLIT_FORMAT_DEPTH24);
		    break;
		default:
		    NVDmaNext(&sd->Card, STRETCH_BLIT_FORMAT_DEPTH8);
		    break;
	    }

	    NVDmaStart(&sd->Card, STRETCH_BLIT_CLIP_POINT, 6);
	    NVDmaNext(&sd->Card, 0x00000000);    // dst_CLip
	    NVDmaNext(&sd->Card, 0xffffffff);    // dst_Clip
	    NVDmaNext(&sd->Card, (msg->destY << 16) | (msg->destX));// dst_y | dst_x
	    NVDmaNext(&sd->Card, (msg->height << 16)| (msg->width));// dst_h | dst_w
	    NVDmaNext(&sd->Card, 1 << 20);  // src_w / dst_w 1:1
	    NVDmaNext(&sd->Card, 1 << 20);  // src_h / dst_h 1:1

	    NVDmaStart(&sd->Card, STRETCH_BLIT_SRC_SIZE, 4);
	    NVDmaNext(&sd->Card, (msg->height << 16) | (msg->width));// src_h | src_w
	    NVDmaNext(&sd->Card, 
		(STRETCH_BLIT_SRC_FORMAT_FILTER_POINT_SAMPLE << 24) |   // BILINEAR | _POINT_SAMPLE
		(STRETCH_BLIT_SRC_FORMAT_ORIGIN_CORNER << 16) |
		(bm_src->pitch));				    // src_pitch
	    NVDmaNext(&sd->Card, bm_src->framebuffer);		    // src_offset
	    NVDmaNext(&sd->Card, ((msg->srcY << 20) & 0xffff0000) 
		    | ((msg->srcX << 4) & 0xffff)); // src_y | src_x

	    NVDmaKickoff(&sd->Card);

	    if (bm_dst->depth == 15)
	    {
		NVDmaStart(&sd->Card, SURFACE_FORMAT, 1);
		NVDmaNext(&sd->Card, SURFACE_FORMAT_DEPTH16);
	    }
	    NVSync(sd);

	    UNLOCK_HW
	    UNLOCK_ALL
	}

/*D(bug("[NVidia] CopyBox(src(%p,%d:%d@%d),dst(%p,%d:%d@%d),%d:%d\n",
		bm_src->framebuffer,msg->srcX,msg->srcY,bm_src->depth,
		bm_dst->framebuffer,msg->destX,msg->destY,bm_dst->depth,
		msg->width, msg->height));
*/	
	bm_src->usecount++;
	bm_dst->usecount++;
    }
}

#define ToRGB555(c) \
    (((c & 0xf80000) >> 9) | ((c & 0xf800) >> 6) | ((c & 0xf8) >> 3) | 0x8000)

#define ToRGB8888(alp,c) ((c) | ((alp) << 24))

static void TransformCursor(struct staticdata *);

static BOOL nv__setcursorshape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    bug("SetCursorShape %p\n", msg->shape);
//    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (msg->shape == NULL)
    {
	NVShowHideCursor(sd, 0);
	sd->Card.cursorVisible = 0;
    }
    else
    {
	OOP_Object	*pfmt;
	OOP_Object	*colormap;
	HIDDT_StdPixFmt	pixfmt;
	HIDDT_Color	color;

	ULONG		width, height, x, y;
	ULONG		maxw,maxh;

	ULONG		*curimg = (ULONG*)((IPTR)sd->Card.CursorStart + (IPTR)sd->Card.FrameBuffer);

	struct pHidd_BitMap_GetPixel __gp = {
	    mID:    OOP_GetMethodID(CLID_Hidd_BitMap, moHidd_BitMap_GetPixel)
	}, *getpixel = &__gp;

	struct pHidd_ColorMap_GetColor __gc = {
	    mID:	    OOP_GetMethodID(CLID_Hidd_ColorMap, moHidd_ColorMap_GetColor),
	    colorReturn:    &color,
	}, *getcolor = &__gc;

	OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
	OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);
	OOP_GetAttr(msg->shape, aHidd_BitMap_PixFmt, (APTR)&pfmt);
	OOP_GetAttr(pfmt, aHidd_PixFmt_StdPixFmt, &pixfmt);
	OOP_GetAttr(msg->shape, aHidd_BitMap_ColorMap, (APTR)&colormap);

	if (sd->Card.alphaCursor)
	{
	    if (width > 64) width = 64;
	    if (height > 64) height = 64;

	    maxw = 64;
	    maxh = 64;
	}
	else
	{
	    if (width > 32) width = 32;
	    if (height > 32) height = 32;

	    maxw = 32;
	    maxh = 32;
	}

	for (y = 0; y < height; y++)
	{
	    for (x = 0; x < width; x++)
	    {
		HIDDT_Pixel pixel;
		getpixel->x = x;
		getpixel->y = y;
		pixel = OOP_DoMethod(msg->shape, (OOP_Msg)getpixel);

		if (pixfmt == vHidd_StdPixFmt_LUT8)
		{
		    getcolor->colorNo = pixel;
		    OOP_DoMethod(colormap, (OOP_Msg)getcolor);
		    pixel = ((color.red << 8) & 0xff0000) |
			    ((color.green) & 0x00ff00)    |
			    ((color.blue >> 8) & 0x0000ff);
		    
		    curimg[maxw*4+4] = pixel ? 0x50000000 : 0x00000000;
		    *curimg++ = pixel;
		}
	    }
	    for (x=width; x < maxw; x++, curimg++)
		if (*curimg!=0x50000000) *curimg = 0;
	}
	for (y=height; y < maxh; y++)
	    for (x=0; x < maxw; x++)
		{ if (*curimg!=0x50000000) *curimg = 0; curimg++; }
    }

    TransformCursor(sd);
    return TRUE;
}

static VOID nv__setcursorvisible(OOP_Class *cl, OOP_Object *o, 
		struct pHidd_Gfx_SetCursorVisible *msg)
{
    NVShowHideCursor(sd, msg->visible);
    sd->Card.cursorVisible = msg->visible;
}

static VOID nv__setcursorpos(OOP_Class *cl, OOP_Object *o,
	        struct pHidd_Gfx_SetCursorPos *msg)
{
    sd->Card.PRAMDAC[0x0300 / 4] = (msg->y << 16) | (msg->x & 0xffff);
}
#undef sd
/* Class related functions */

static void TransformCursor(struct staticdata *sd)
{
    ULONG *tmp = AllocPooled(sd->memPool, 4 * 64 * 64);
    ULONG dwords,i;
    ULONG *curimg = (ULONG*)((IPTR)sd->Card.CursorStart + (IPTR)sd->Card.FrameBuffer);


    if (sd->Card.alphaCursor)
    {
	dwords = 64*64;
	for (i=0; i < dwords; i++)
	{
	    UBYTE alp;
	    if (curimg[i] == 0) alp = 0;
	    else alp = 0xe0;
	    
	    if (curimg[i] == 0x50000000) ((ULONG*)tmp)[i] = ToRGB8888(0x50,0,0,0);
	    else ((ULONG*)tmp)[i] = ToRGB8888(alp, curimg[i]);
	}
    }
    else
    {
	dwords = (32*32) >> 1;

	for(i=0; i < dwords; i++)
	{
	    ((UWORD*)tmp)[i] = ToRGB555(curimg[i]);
	}
    }


    for (i=0; i < dwords; i++)
	sd->Card.CURSOR[i] = tmp[i];

    FreePooled(sd->memPool, tmp, 4*64*64);
}


#define NUM_ROOT_METHODS    3
#define	NUM_GFX_METHODS	    6

OOP_Class *init_nvclass(struct staticdata *sd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] = {
	{ OOP_METHODDEF(nv__new),   moRoot_New },
	{ OOP_METHODDEF(nv__get),   moRoot_Get },
	{ OOP_METHODDEF(nv__set),   moRoot_Set },
	{ NULL, 0 }
    };

    struct OOP_MethodDescr gfx_descr[NUM_GFX_METHODS + 1] = {
	{ OOP_METHODDEF(nv__newbitmap),	moHidd_Gfx_NewBitMap },
	{ OOP_METHODDEF(nv__show),	moHidd_Gfx_Show },
	{ OOP_METHODDEF(nv__copybox),	moHidd_Gfx_CopyBox },
	{ OOP_METHODDEF(nv__setcursorvisible),	moHidd_Gfx_SetCursorVisible },
	{ OOP_METHODDEF(nv__setcursorpos),	moHidd_Gfx_SetCursorPos },
	{ OOP_METHODDEF(nv__setcursorshape),	moHidd_Gfx_SetCursorShape },
	{ NULL, 0 }
    };

    struct OOP_InterfaceDescr ifdescr[] = {
	{ root_descr,	IID_Root,	NUM_ROOT_METHODS },
	{ gfx_descr,	IID_Hidd_Gfx,	NUM_GFX_METHODS	 },
	{ NULL, NULL, 0 }
    };

    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] = {
	{ aMeta_SuperID,	(IPTR)CLID_Hidd_Gfx },
	{ aMeta_InterfaceDescr,	(IPTR)ifdescr },
	{ aMeta_InstSize,	0 },
	{ aMeta_ID,		(IPTR)CLID_Hidd_Gfx_nVidia },
	{ TAG_DONE, 0UL }
    };

    EnterFunc(bug("[NVidia] " CLID_Hidd_Gfx_nVidia " class init.\n"));

    if (MetaAttrBase)
    {
	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);

	if (cl)
	{
	    cl->UserData = sd;
	    sd->nvclass = cl;

	    OOP_AddClass(cl);
	}
	
	OOP_ReleaseAttrBase(IID_Meta);
    }

    D(bug("[NVidia] init_nvclass=%p\n", cl));

    return cl;
}

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
    
    result = (IPTR)Allocate(sd->CardMem, ((width * bpp + 63) & ~63) * height);

    D(bug("[NVidia] AllocBitmapArea(%dx%d@%d) = %p\n",
	width, height, bpp, result));
    /*
	If Allocate failed, make the 0xffffffff as return. If it succeeded, make
	the memory pointer relative to the begin of GFX memory
    */
    if (result == 0) --result;
    else result -= (IPTR)sd->Card.FrameBuffer;

    /* Generic thing. Will be extended later */
    return result;
}

VOID FreeBitmapArea(struct staticdata *sd, IPTR bmp, ULONG width, ULONG height,
    ULONG bpp)
{
    APTR ptr = (APTR)(bmp + sd->Card.FrameBuffer);

    D(bug("[NVidia] FreeBitmapArea(%p,%dx%d@%d)\n",
	bmp, width, height, bpp));

    Deallocate(sd->CardMem, ptr, ((width * bpp + 63) & ~63) * height);
}



/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <exec/libraries.h>
#include <exec/rawfmt.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <graphics/displayinfo.h>
#include <aros/libcall.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include <aros/symbolsets.h>

#include "amigavideogfx.h"
#include "amigavideobitmap.h"

#include "chipset.h"
#include "blitter.h"

#include LC_LIBDEFS_FILE

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define ADDTAG(tag,data) \
	*tagptr++ = tag; \
	*tagptr++ = data;


OOP_Object *AmigaVideoCl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
	struct amigavideo_staticdata *csd = CSD(cl);
	struct TagItem mytags[2];
	struct pRoot_New mymsg;
	ULONG allocsize = 2000;
	ULONG allocedsize = 0;
	UBYTE i;

	ULONG *tags, *tagptr;

	ULONG *tags_640_512, *tags_640_400, *tags_640_256, *tags_640_200;
	ULONG *tags_320_512, *tags_320_400, *tags_320_256, *tags_320_200;
	ULONG *pftags_aga[8], *pftags_ecs_hires[4], *pftags_ecs_lores[5];
	ULONG *mode_tags_aga, *mode_tags_ecs;

	if (csd->initialized)
		return NULL;
   
	tags = tagptr = AllocVec(allocsize, MEMF_PUBLIC);

	tags_640_512 = tagptr;
	ADDTAG(aHidd_Sync_HDisp, 640);
	ADDTAG(aHidd_Sync_VDisp, 512);
	ADDTAG(aHidd_Sync_Flags, vHidd_Sync_Interlaced);
	ADDTAG(TAG_DONE, 0);
	tags_640_400 = tagptr;
	ADDTAG(aHidd_Sync_HDisp, 640);
	ADDTAG(aHidd_Sync_VDisp, 400);
	ADDTAG(aHidd_Sync_Flags, vHidd_Sync_Interlaced);
	ADDTAG(TAG_DONE, 0);
	tags_640_256 = tagptr;
	ADDTAG(aHidd_Sync_HDisp, 640);
	ADDTAG(aHidd_Sync_VDisp, 256);
	ADDTAG(aHidd_Sync_Flags, 0);
	ADDTAG(TAG_DONE, 0);
	tags_640_200 = tagptr;
	ADDTAG(aHidd_Sync_HDisp, 640);
	ADDTAG(aHidd_Sync_VDisp, 200);
	ADDTAG(aHidd_Sync_Flags, 0);
	ADDTAG(TAG_DONE, 0);

	tags_320_512 = tagptr;
	ADDTAG(aHidd_Sync_HDisp, 320);
	ADDTAG(aHidd_Sync_VDisp, 512);
	ADDTAG(aHidd_Sync_Flags, vHidd_Sync_Interlaced);
	ADDTAG(TAG_DONE, 0);
	tags_320_400 = tagptr;
	ADDTAG(aHidd_Sync_HDisp, 320);
	ADDTAG(aHidd_Sync_VDisp, 400);
	ADDTAG(aHidd_Sync_Flags, vHidd_Sync_Interlaced);
	ADDTAG(TAG_DONE, 0);
	tags_320_256 = tagptr;
	ADDTAG(aHidd_Sync_HDisp, 320);
	ADDTAG(aHidd_Sync_VDisp, 256);
	ADDTAG(aHidd_Sync_Flags, 0);
	ADDTAG(TAG_DONE, 0);
	tags_320_200 = tagptr;
	ADDTAG(aHidd_Sync_HDisp, 320);
	ADDTAG(aHidd_Sync_VDisp, 200);
	ADDTAG(aHidd_Sync_Flags, 0);
	ADDTAG(TAG_DONE, 0);

	if (csd->aga) {
		for (i = 0; i < 8; i++) {
			pftags_aga[i] = tagptr;
			ADDTAG(aHidd_PixFmt_RedShift,		 8);
			ADDTAG(aHidd_PixFmt_GreenShift, 	16);
			ADDTAG(aHidd_PixFmt_BlueShift,		24);
			ADDTAG(aHidd_PixFmt_AlphaShift,		 0);
			ADDTAG(aHidd_PixFmt_RedMask,		0x00FF0000);
			ADDTAG(aHidd_PixFmt_GreenMask,		0x0000FF00);
			ADDTAG(aHidd_PixFmt_BlueMask,		0x000000FF);
			ADDTAG(aHidd_PixFmt_AlphaMask,		0x00000000);
			ADDTAG(aHidd_PixFmt_CLUTMask,		0x000000FF);
			ADDTAG(aHidd_PixFmt_CLUTShift,		0);
			ADDTAG(aHidd_PixFmt_ColorModel,		vHidd_ColorModel_Palette);
			ADDTAG(aHidd_PixFmt_Depth,		i + 1);
			ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
			ADDTAG(aHidd_PixFmt_BitsPerPixel,	i + 1);
			ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
			ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
			ADDTAG(TAG_DONE, 0);
		}
		mode_tags_aga = tagptr;
		ADDTAG(aHidd_Sync_HMin,		112);
		ADDTAG(aHidd_Sync_VMin,		112);
		ADDTAG(aHidd_Sync_HMax,		16384);
		ADDTAG(aHidd_Sync_VMax,		16384);
		ADDTAG(aHidd_Sync_Description,	(IPTR)"MODE: %hx%v");
#if 1
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_aga[0]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_aga[1]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_aga[2]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_aga[3]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_aga[4]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_aga[5]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_aga[6]);
#endif
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_aga[7]);

		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_320_200);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_320_256);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_320_512);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_320_400);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_640_200);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_640_256);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_640_512);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_640_400);
		ADDTAG(TAG_DONE, 0);

    		mytags[0].ti_Tag = aHidd_Gfx_ModeTags;
    		mytags[0].ti_Data = (IPTR)mode_tags_aga;
	} else {
		for (i = 0; i < 5; i++) {
			pftags_ecs_lores[i] = tagptr;
			ADDTAG(aHidd_PixFmt_RedShift,		20);
			ADDTAG(aHidd_PixFmt_GreenShift, 	24);
			ADDTAG(aHidd_PixFmt_BlueShift,		28);
			ADDTAG(aHidd_PixFmt_AlphaShift,		 0);
			ADDTAG(aHidd_PixFmt_RedMask,		0x00000F00);
			ADDTAG(aHidd_PixFmt_GreenMask,		0x000000F0);
			ADDTAG(aHidd_PixFmt_BlueMask,		0x0000000F);
			ADDTAG(aHidd_PixFmt_AlphaMask,		0x00000000);
			ADDTAG(aHidd_PixFmt_CLUTMask,		0x0000001F);
			ADDTAG(aHidd_PixFmt_CLUTShift,		0);
			ADDTAG(aHidd_PixFmt_ColorModel,		vHidd_ColorModel_Palette);
			ADDTAG(aHidd_PixFmt_Depth,		i + 1);
			ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
			ADDTAG(aHidd_PixFmt_BitsPerPixel,	i + 1);
			ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
			ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
			ADDTAG(TAG_DONE, 0);
		}
		for (i = 0; i < 4; i++) {
			pftags_ecs_hires[i] = tagptr;
			ADDTAG(aHidd_PixFmt_RedShift,		20);
			ADDTAG(aHidd_PixFmt_GreenShift, 	24);
			ADDTAG(aHidd_PixFmt_BlueShift,		28);
			ADDTAG(aHidd_PixFmt_AlphaShift,		 0);
			ADDTAG(aHidd_PixFmt_RedMask,		0x00000F00);
			ADDTAG(aHidd_PixFmt_GreenMask,		0x000000F0);
			ADDTAG(aHidd_PixFmt_BlueMask,		0x0000000F);
			ADDTAG(aHidd_PixFmt_AlphaMask,		0x00000000);
			ADDTAG(aHidd_PixFmt_CLUTMask,		0x0000001F);
			ADDTAG(aHidd_PixFmt_CLUTShift,		0);
			ADDTAG(aHidd_PixFmt_ColorModel,		vHidd_ColorModel_Palette);
			ADDTAG(aHidd_PixFmt_Depth,		i + 1);
			ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
			ADDTAG(aHidd_PixFmt_BitsPerPixel,	i + 1);
			ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
			ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
			ADDTAG(TAG_DONE, 0);
		}
		mode_tags_ecs = tagptr;
		ADDTAG(aHidd_Sync_HMin,		112);
		ADDTAG(aHidd_Sync_VMin,		112);
		ADDTAG(aHidd_Sync_HMax,		16384);
		ADDTAG(aHidd_Sync_VMax,		16384);
		ADDTAG(aHidd_Sync_Description,	(IPTR)"MODE: %hx%v");
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_hires[0]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_hires[1]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_hires[2]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_hires[3]);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_640_200);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_640_256);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_640_512);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_640_400);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_lores[0]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_lores[1]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_lores[2]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_lores[3]);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_lores[4]);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_320_200);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_320_256);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_320_512);
		ADDTAG(aHidd_Gfx_SyncTags,	(IPTR)tags_320_400);
		ADDTAG(TAG_DONE, 0);

     		mytags[0].ti_Tag = aHidd_Gfx_ModeTags;
    		mytags[0].ti_Data = (IPTR)mode_tags_ecs;
	}

    allocedsize = (ULONG)tagptr - (ULONG)tags;
    bug("alloc=%d alloced=%d\n", allocsize, allocedsize);

    mytags[1].ti_Tag = TAG_MORE;
    mytags[1].ti_Data = (IPTR)msg->attrList;

    EnterFunc(bug("AGFX::New()\n"));

    mymsg.mID	= msg->mID;
    mymsg.attrList = mytags;
    msg = &mymsg;

    /* Register gfxmodes */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL != o)
    {
	struct amigagfx_data *data = OOP_INST_DATA(cl, o);
	D(bug("AGFX::New(): Got object from super\n"));
	NewList((struct List *)&data->bitmaps);
	csd->initialized = 1;
    }
    FreeVec(tags);
    ReturnPtr("AGFX::New", OOP_Object *, o);
}

/********** GfxHidd::Dispose()  ******************************/
VOID AmigaVideoCl__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct amigagfx_data *data;
    
    EnterFunc(bug("AGFX::Dispose(o=%p)\n", o));
    
    data = OOP_INST_DATA(cl, o);
    
    D(bug("AGFX::Dispose: calling super\n"));    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("AGFX::Dispose");
}


OOP_Object *AmigaVideoCl__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{  
    struct amigavideo_staticdata *csd = CSD(cl);
    HIDDT_ModeID		modeid;
    struct pHidd_Gfx_NewBitMap   newbitmap;
    struct TagItem tags[2];
   
    EnterFunc(bug("AGFX::NewBitMap()\n"));
    
    modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    D(bug("modeid=%08x\n", modeid));
    if (modeid != vHidd_ModeID_Invalid) {
	tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
	tags[0].ti_Data = (IPTR)CSD(cl)->amigabmclass;
	tags[1].ti_Tag = TAG_MORE;
	tags[1].ti_Data = (IPTR)msg->attrList;
	newbitmap.mID = msg->mID;
	newbitmap.attrList = tags;
	msg = &newbitmap;
    }

    ReturnPtr("AGFX::NewBitMap", OOP_Object *, (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg));
}

VOID AmigaVideoCl__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    ULONG idx;
    
    //bug("AmigaVideoCl__Root__Get %x\n", msg->attrID);

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
    	//bug("=%x\n", idx);
    	switch (idx)
    	{
    	    case aoHidd_Gfx_HWSpriteTypes:
	    	*msg->storage = vHidd_SpriteType_3Plus1;
	    return;
	    case aoHidd_Gfx_SupportsHWCursor:
	    case aoHidd_Gfx_NoFrameBuffer:
	    	*msg->storage = TRUE;
	    return;
	    case aoHidd_Gfx_IsWindowed:
	    	*msg->storage = FALSE;
	    return;
	    case aoHidd_Gfx_DriverName:
		*msg->storage = (IPTR)"AmigaVideo";
	    return;
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID AmigaVideoCl__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
#if 0
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigagfx_data *data = OOP_INST_DATA(cl, obj);
    struct TagItem  	    *tag;
    const struct TagItem    *tstate;
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
    	ULONG idx;
	bug("AmigaVideoCl__Root__Set %x\n", tag->ti_Tag);
    	if (IS_GFX_ATTR(tag->ti_Tag, idx)) {
    	    bug("->%d\n", idx);
    	}
    }
#endif
    OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
}

OOP_Object *AmigaVideoCl__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);

    D(bug("SHOW %x\n", msg->bitMap));

    if (msg->bitMap) {
    	IPTR tags[] = {aHidd_BitMap_Visible, TRUE, TAG_DONE};
    	OOP_Object *gfxhidd, *sync, *pf;
    	IPTR modeid = vHidd_ModeID_Invalid;
    	IPTR dwidth, dheight, dflags;

	OOP_GetAttr(msg->bitMap, aHidd_BitMap_ModeID , &modeid);
    	OOP_GetAttr(msg->bitMap, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
	HIDD_Gfx_GetMode(gfxhidd, modeid, &sync, &pf);
	OOP_GetAttr(sync, aHidd_Sync_HDisp, &dwidth);
	OOP_GetAttr(sync, aHidd_Sync_VDisp, &dheight);
	OOP_GetAttr(sync, aHidd_Sync_Flags, &dflags);

	csd->interlace = (dflags & vHidd_Sync_Interlaced) ? 1 : 0;
	csd->res = dwidth < 400 ? 0 : (dwidth > 800 ? 2 : 1);
	OOP_SetAttrs(msg->bitMap, (struct TagItem *)tags);
    }
    return msg->bitMap;
}

VOID AmigaVideoCl__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    IPTR src, dst;
    BOOL ok = FALSE;
 
    OOP_GetAttr(msg->src,  aHidd_AmigaVideoBitMap_Drawable, &src);
    OOP_GetAttr(msg->dest, aHidd_AmigaVideoBitMap_Drawable, &dst);
    if (src && dst) {
        struct amigabm_data *sdata = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
    	struct amigabm_data *ddata = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
    	ok = blit_copybox(csd, sdata, ddata, msg->srcX, msg->srcY, msg->width, msg->height, msg->destX, msg->destY, mode);
    }
    if (!ok)
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);   
}

BOOL AmigaVideoCl__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *shape, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    IPTR width, height;

    OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);

    return setsprite(csd, width, height, msg);
}
                             
BOOL AmigaVideoCl__Hidd_Gfx__GetMaxSpriteSize(OOP_Class *cl, ULONG Type, ULONG *Width, ULONG *Height)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    *Width = csd->aga ? 64 : 16;
    *Height = *Width * 2;
    return TRUE;
}
BOOL AmigaVideoCl__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    setspritepos(csd, msg->x, msg->y);
    return TRUE;
}
VOID AmigaVideoCl__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    setspritevisible(csd, msg->visible);
}

static void freeattrbases(struct amigavideo_staticdata *csd)
{
    OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    OOP_ReleaseAttrBase(IID_Hidd_PlanarBM);
    OOP_ReleaseAttrBase(IID_Hidd_AmigaVideoBitMap);
    OOP_ReleaseAttrBase(IID_Hidd_GC);
    OOP_ReleaseAttrBase(IID_Hidd_Sync);
    OOP_ReleaseAttrBase(IID_Hidd_Gfx);
    OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
    OOP_ReleaseAttrBase(IID_Hidd_ColorMap);
}

int Init_AmigaVideoClass(LIBBASETYPEPTR LIBBASE)
{
    struct amigavideo_staticdata *csd = &LIBBASE->csd;
    D(bug("Init_AmigaVideoClass\n"));
    __IHidd_BitMap  	= OOP_ObtainAttrBase(IID_Hidd_BitMap);
    __IHidd_PlanarBM  	= OOP_ObtainAttrBase(IID_Hidd_PlanarBM);
    __IHidd_AmigaVideoBitmap  	= OOP_ObtainAttrBase(IID_Hidd_AmigaVideoBitMap);
    __IHidd_GC      	= OOP_ObtainAttrBase(IID_Hidd_GC);
    __IHidd_Sync    	= OOP_ObtainAttrBase(IID_Hidd_Sync);
    __IHidd_Gfx     	= OOP_ObtainAttrBase(IID_Hidd_Gfx);
    __IHidd_PixFmt		= OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    __IHidd_ColorMap 	= OOP_ObtainAttrBase(IID_Hidd_ColorMap);
    
    if (!__IHidd_BitMap || !__IHidd_PlanarBM || !__IHidd_AmigaVideoBitmap || !__IHidd_GC ||
    	!__IHidd_Sync || !__IHidd_Gfx || !__IHidd_PixFmt || !__IHidd_ColorMap)
    {
    	D(bug("Init_AmigaVideoClass fail\n"));
    	freeattrbases(csd);
    	return 0;
    }
    return TRUE;
}


static int Expunge_AmigaVideoClass(LIBBASETYPEPTR LIBBASE)
{
    struct amigavideo_staticdata *csd = &LIBBASE->csd;
    D(bug("Expunge_AmigaVideoClass\n"));
    freeattrbases(csd);
    return TRUE;
}

ADD2EXPUNGELIB(Expunge_AmigaVideoClass, 1)

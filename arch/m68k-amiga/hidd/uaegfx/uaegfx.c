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

#include "uaegfx.h"
#include "uaegfxbitmap.h"
#include "uaertg.h"

#include LC_LIBDEFS_FILE

#define SDEBUG 0
#define DEBUG 1
#include <aros/debug.h>

#define SIZE_RESLIST 4
#define SIZE_PFLIST 17
#define SIZE_MODELIST (5 + RGBFB_MaxFormats)

struct RTGFormat
{
    UWORD rgbformat;
    ULONG rm, gm, bm;
    UWORD rs, gs, bs;
};

static const struct RTGFormat formats[] =
{
    { RGBFB_CLUT,	0x00ff0000, 0x0000ff00, 0x000000ff,   8, 16, 24 },
    { RGBFB_B8G8R8A8,	0x0000ff00, 0x00ff0000, 0xff000000,  16,  8,  0 },
    { RGBFB_A8R8G8B8,	0x00ff0000, 0x0000ff00, 0x000000ff,   8, 16, 24 },
    { RGBFB_A8B8G8R8,	0x000000ff, 0x0000ff00, 0x00ff0000,  24, 16,  8 },
    { RGBFB_R8G8B8A8,	0xff000000, 0x00ff0000, 0x0000ff00,   0,  8, 16 },
    { 0 }
};

OOP_Object *UAEGFXCl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct LibResolution *r;
    struct Node *node;
    WORD rescnt, i, j, k, l, depth;
    struct TagItem *reslist, *restags, *pflist, *modetags;
    struct pRoot_New mymsg;
    struct TagItem mytags[2];

    if (csd->initialized)
	return NULL;

    node = (struct Node*)(((ULONG*)(csd->boardinfo + PSSO_BoardInfo_ResolutionsList))[0]);
    r = (struct LibResolution*)node->ln_Succ;
    rescnt = 0;
    while (r->node.ln_Succ) {
        rescnt++;
    	r = (struct LibResolution*)r->node.ln_Succ;
    }
    D(bug("UAEGFX: resolutions: %d\n", rescnt));

    reslist = AllocVec(rescnt * SIZE_RESLIST * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);
    restags = AllocVec((rescnt + 1) * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);
    pflist = AllocVec(RGBFB_MaxFormats * SIZE_PFLIST * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);
    modetags = AllocVec(SIZE_MODELIST * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);
    
    r = (struct LibResolution*)node->ln_Succ;
    for (j = 0, i = 0; i < rescnt; i++, j++) {
    	reslist[i * SIZE_RESLIST + 0].ti_Tag = aHidd_Sync_HDisp;
    	reslist[i * SIZE_RESLIST + 0].ti_Data = r->Width;
    	reslist[i * SIZE_RESLIST + 1].ti_Tag = aHidd_Sync_VDisp;
    	reslist[i * SIZE_RESLIST + 1].ti_Data = r->Height;
    	reslist[i * SIZE_RESLIST + 2].ti_Tag = aHidd_Sync_Description;
    	reslist[i * SIZE_RESLIST + 2].ti_Data = (IPTR)"UAEGFX:%hx%v";
    	reslist[i * SIZE_RESLIST + 3].ti_Tag = TAG_DONE;
    	reslist[i * SIZE_RESLIST + 3].ti_Data = 0;
    	D(bug("%08x %d*%d\n", r, r->Width, r->Height));
    	restags[j].ti_Tag = aHidd_Gfx_SyncTags;
    	restags[j].ti_Data = (IPTR)&reslist[i * SIZE_RESLIST];
    	r = (struct LibResolution*)r->node.ln_Succ;
    }
    restags[j].ti_Tag = TAG_DONE;
    restags[j].ti_Data = 0;
    
    k = 0;
    for (i = 0, j = 0; i < RGBFB_MaxFormats; i++) {
   	depth = getrtgdepth(1 << i);
    	if (!((1 << i) & RGBFB_SUPPORTMASK) || depth == 0) {
      	    pflist[j].ti_Tag = TAG_DONE;
     	    pflist[j].ti_Data = 0;
    	    j++;
    	    continue;
    	}
    	for (l = 0; formats[l].rgbformat; l++) {
    	    if (formats[l].rgbformat == i)
    	    	break;
    	}
    	if (formats[l].rgbformat == 0) {
       	    pflist[j].ti_Tag = TAG_DONE;
     	    pflist[j].ti_Data = 0;
    	    j++;
    	    continue;
    	}
   	D(bug("RTGFORMAT=%d added. Depth=%d\n", i, depth));

    	modetags[k].ti_Tag = aHidd_Gfx_PixFmtTags;
    	modetags[k].ti_Data = (IPTR)&pflist[j];
    	k++;
    	    
    	pflist[j].ti_Tag = aHidd_PixFmt_RedShift;
     	pflist[j].ti_Data = formats[l].rs;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_GreenShift;
     	pflist[j].ti_Data = formats[l].gs;
    	j++;
     	pflist[j].ti_Tag = aHidd_PixFmt_BlueShift;
     	pflist[j].ti_Data = formats[l].bs;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_AlphaShift;
     	pflist[j].ti_Data = 0;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_RedMask;
     	pflist[j].ti_Data = formats[l].rm;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_GreenMask;
     	pflist[j].ti_Data = formats[l].gm;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_BlueMask;
     	pflist[j].ti_Data = formats[l].bm;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_AlphaMask;
     	pflist[j].ti_Data = 0x00000000;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_CLUTMask;
     	pflist[j].ti_Data = 0x000000FF;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_CLUTShift;
     	pflist[j].ti_Data = 0;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_ColorModel;
     	pflist[j].ti_Data = depth <= 8 ? vHidd_ColorModel_Palette : vHidd_ColorModel_TrueColor;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_Depth;
     	pflist[j].ti_Data = depth;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_BytesPerPixel;
     	pflist[j].ti_Data = (depth + 7) / 8;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_BitsPerPixel;
     	pflist[j].ti_Data = depth;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_StdPixFmt;
     	pflist[j].ti_Data = vHidd_StdPixFmt_Native;
    	j++;
    	pflist[j].ti_Tag = aHidd_PixFmt_BitMapType;
     	pflist[j].ti_Data = vHidd_BitMapType_Chunky;
    	j++;
     	pflist[j].ti_Tag = TAG_DONE;
     	pflist[j].ti_Data = 0;
    	j++;
    }
 
    modetags[k].ti_Tag = aHidd_Sync_HMin;
    modetags[k].ti_Data = 112;
    k++;
    modetags[k].ti_Tag = aHidd_Sync_VMin;
    modetags[k].ti_Data = 112;
    k++;
    modetags[k].ti_Tag = aHidd_Sync_HMax;
    modetags[k].ti_Data = 16384;
    k++;
    modetags[k].ti_Tag = aHidd_Sync_VMax;
    modetags[k].ti_Data = 16384;
    k++;
    modetags[k].ti_Tag = TAG_MORE;
    modetags[k].ti_Data = (IPTR)restags;
 
    mytags[0].ti_Tag = aHidd_Gfx_ModeTags;
    mytags[0].ti_Data = (IPTR)modetags;
    mytags[1].ti_Tag = TAG_MORE;
    mytags[1].ti_Data = (IPTR)msg->attrList;
 
    EnterFunc(bug("UAEGFX::New() tags=%x\n", mytags));

    mymsg.mID	= msg->mID;
    mymsg.attrList = mytags;
    msg = &mymsg;

    /* Register gfxmodes */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL != o)
    {
	struct gfx_data *data = OOP_INST_DATA(cl, o);
	D(bug("UAEGFX::New(): Got object from super\n"));
	NewList((struct List *)&data->bitmaps);
	csd->initialized = 1;
	csd->spritecolors = 16;
    }
    
    FreeVec(restags);
    FreeVec(reslist);
    FreeVec(pflist);
    FreeVec(modetags);
    
    ReturnPtr("UAEGFX::New", OOP_Object *, o);
}

/********** GfxHidd::Dispose()  ******************************/
VOID UAEGFXCl__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct gfx_data *data;
    
    EnterFunc(bug("UAEGFX::Dispose(o=%p)\n", o));
    
    data = OOP_INST_DATA(cl, o);
    
    D(bug("UAEGFX::Dispose: calling super\n"));    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("UAEGFX::Dispose");
}


OOP_Object *UAEGFXCl__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{  
    struct uaegfx_staticdata *csd = CSD(cl);
    HIDDT_ModeID		modeid;
    struct pHidd_Gfx_NewBitMap   newbitmap;
    OOP_Object			*newbm;
    HIDDT_StdPixFmt		 stdpf;
    struct gfx_data 	*data = OOP_INST_DATA(cl, o);
    struct TagItem tags[2];
   
    EnterFunc(bug("UAEGFX::NewBitMap()\n"));
    
    modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    if (modeid != vHidd_ModeID_Invalid) {
	tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
	tags[0].ti_Data = (IPTR)CSD(cl)->bmclass;
	tags[1].ti_Tag = TAG_MORE;
	tags[1].ti_Data = (IPTR)msg->attrList;
	newbitmap.mID = msg->mID;
	newbitmap.attrList = tags;
	msg = &newbitmap;
    }

    ReturnPtr("UAEGFX::NewBitMap", OOP_Object *, (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg));
}

VOID UAEGFXCl__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    ULONG idx;
    
    //bug("UAEGFXCl__Root__Get %x\n", msg->attrID);

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
    	//bug("=%x\n", idx);
    	switch (idx)
    	{
    	    case aoHidd_Gfx_HWSpriteTypes:
	    	*msg->storage = csd->hardwaresprite ? vHidd_SpriteType_3Plus1 : 0;
	    return;
	    case aoHidd_Gfx_SupportsHWCursor:
	    	*msg->storage = csd->hardwaresprite;
	    return;
	    case aoHidd_Gfx_NoFrameBuffer:
	    	*msg->storage = TRUE;
	    return;
	    case aoHidd_Gfx_IsWindowed:
	    	*msg->storage = FALSE;
	    return;
	    case aoHidd_Gfx_DriverName:
		*msg->storage = (IPTR)"UAEGFX";
	    return;
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID UAEGFXCl__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct gfx_data *data = OOP_INST_DATA(cl, obj);
    struct TagItem  	    *tag;
    const struct TagItem    *tstate;
#if 0
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
    	ULONG idx;
	bug("UAEGFXCl__Root__Set %x\n", tag->ti_Tag);
    	if (IS_GFX_ATTR(tag->ti_Tag, idx)) {
    	    bug("->%d\n", idx);
    	}
    }
#endif
    OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
}

OOP_Object *UAEGFXCl__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct gfx_data *data = OOP_INST_DATA(cl, o);

    D(bug("SHOW %x\n", msg->bitMap));

    if (msg->bitMap) {
    	IPTR tags[] = {aHidd_BitMap_Visible, TRUE, TAG_DONE};
        OOP_SetAttrs(msg->bitMap, (struct TagItem *)tags);
    } else {
    	SetDisplay(csd, FALSE);
    }
    return msg->bitMap;
}

VOID UAEGFXCl__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    struct bm_data *sdata = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
    struct bm_data *ddata = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
    struct RenderInfo risrc, ridst;

    if (sdata->rgbformat != ddata->rgbformat) {
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    	return;
    }
    makerenderinfo(csd, &risrc, sdata);
    makerenderinfo(csd, &ridst, ddata);
    if (!BlitRectNoMaskComplete(csd, &risrc, &ridst,
    	msg->srcX, msg->srcY, msg->destX, msg->destY,
    	msg->width, msg->height, modetable[mode], sdata->rgbformat))
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL UAEGFXCl__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *shape, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    IPTR width, height;
    WORD x, y, hiressprite;
    UWORD *p;
    ULONG flags;

    OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);
    Forbid();
    pb(csd->boardinfo + PSSO_BoardInfo_MouseXOffset, msg->xoffset);
    pb(csd->boardinfo + PSSO_BoardInfo_MouseYOffset, msg->yoffset);
    p = (UWORD*)gl(csd->boardinfo + PSSO_BoardInfo_MouseImage);
    if (p == NULL || width != csd->sprite_width || height != csd->sprite_height) {
    	FreeVec(gl(csd->boardinfo + PSSO_BoardInfo_MouseImage));
    	p = AllocVec(4 + 4 + ((width + 15) & ~15) / 8 * height * 2, MEMF_CLEAR | MEMF_PUBLIC);
    	if (!p) {
    	    Permit();
    	    return FALSE;
    	}
        pl(csd->boardinfo + PSSO_BoardInfo_MouseImage, (ULONG)p);
        csd->sprite_width = width;
        csd->sprite_height = height;
    }

    flags = gl(csd->boardinfo + PSSO_BoardInfo_Flags);
    flags &= ~(1 << BIB_HIRESSPRITE);
    hiressprite = 1;
    if (width > 16) {
    	flags |= 1 << BIB_HIRESSPRITE;
    	hiressprite = 2;
    }
    pl(csd->boardinfo + PSSO_BoardInfo_Flags, flags);

    pb(csd->boardinfo + PSSO_BoardInfo_MouseWidth, width / hiressprite);
    pb(csd->boardinfo + PSSO_BoardInfo_MouseHeight, height);

    p += 2 * hiressprite;
    for(y = 0; y < height; y++) {
    	UWORD pix1 = 0, pix2 = 0, xcnt = 0;
    	for(x = 0; x < width; x++) {
    	    UBYTE c = HIDD_BM_GetPixel(msg->shape, x, y);
    	    pix1 <<= 1;
    	    pix2 <<= 1;
    	    pix1 |= (c & 1) ? 1 : 0;
    	    pix2 |= (c & 2) ? 1 : 0;
    	    xcnt++;
    	    if (xcnt == 15) {
    	    	xcnt = 0;
    	    	p[x / 16] = pix1;
    	    	p[width / 16 + x / 16] = pix2;
    	    }
    	}
    	p += (width / 16) * 2;
    }
    Permit();
    SetSpriteImage(csd);
    return TRUE;
}
                             
BOOL UAEGFXCl__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    pw(csd->boardinfo + PSSO_BoardInfo_MouseX, msg->x);
    pw(csd->boardinfo + PSSO_BoardInfo_MouseY, msg->y);
    SetSpritePosition(csd);
    return TRUE;
}
VOID UAEGFXCl__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    SetSprite(csd, msg->visible);
}

BOOL UAEGFXCl__Hidd_Gfx__CheckMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CheckMode *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    IPTR width, height, bpp;
    OOP_GetAttr(msg->sync, aHidd_Sync_HDisp, &width);
    OOP_GetAttr(msg->sync, aHidd_Sync_VDisp, &height);
    OOP_GetAttr(msg->pixFmt, aHidd_PixFmt_BytesPerPixel, &bpp);
    return width * height * bpp < csd->vram_size;
}


static void freeattrbases(struct uaegfx_staticdata *csd)
{
    OOP_ReleaseAttrBase(__IHidd_BitMap);
    OOP_ReleaseAttrBase(__IHidd_UAEGFXBitmap);
    OOP_ReleaseAttrBase(__IHidd_GC);
    OOP_ReleaseAttrBase(__IHidd_Sync);
    OOP_ReleaseAttrBase(__IHidd_Gfx);
    OOP_ReleaseAttrBase(__IHidd_PixFmt);
    OOP_ReleaseAttrBase(__IHidd_ColorMap);
}

BOOL Init_UAEGFXClass(LIBBASETYPEPTR LIBBASE)
{
    struct uaegfx_staticdata *csd = &LIBBASE->csd;
    struct MemChunk *mc;
    ULONG size;

    D(bug("Init_UAEGFXClass\n"));
    csd->uaeromvector = (APTR)(0xf00000 + 0xff60);
    if ((gl(csd->uaeromvector) & 0xff00ffff) != 0xa0004e75) {
    	D(bug("UAE boot ROM entry point not found. UAEGFX not enabled.\n"));
    	return FALSE;
    }
    csd->boardinfo = AllocVec(PSSO_BoardInfo_SizeOf + PSSO_BitMapExtra_Last, MEMF_CLEAR | MEMF_PUBLIC);
    if (!csd->boardinfo)
    	return FALSE;
    NEWLIST((struct List*)(csd->boardinfo + PSSO_BoardInfo_ResolutionsList));
    csd->bitmapextra = csd->boardinfo + PSSO_BoardInfo_SizeOf;
    pl(csd->boardinfo + PSSO_BoardInfo_BitMapExtra, (ULONG)csd->bitmapextra);
    if (!FindCard(csd)) {
    	D(bug("FindCard() returned false\n"));
    	FreeVec(csd->boardinfo);
    	return FALSE;
    }
    D(bug("FindCard done\n"));
    InitCard(csd);
    if (IsListEmpty((struct List*)(csd->boardinfo + PSSO_BoardInfo_ResolutionsList))) {
     	D(bug("InitCard() failed\n"));
    	FreeVec(csd->boardinfo);
    	return FALSE;
    }
    D(bug("InitCard done\n"));

    csd->hardwaresprite = SetSprite(csd, FALSE) && (gl(csd->boardinfo + PSSO_BoardInfo_Flags) & (1 << BIB_HARDWARESPRITE));
    bug("hardware sprite: %d\n", csd->hardwaresprite);

    csd->vram_start = (UBYTE*)gl(csd->boardinfo + PSSO_BoardInfo_MemoryBase);
    csd->vram_size = gl(csd->boardinfo + PSSO_BoardInfo_MemorySize);
    D(bug("UAE RTG found at %08x size %08x\n", csd->vram_start, csd->vram_size));

    size = (csd->vram_size - 1) & 0xffff0000;
    mc = (struct MemChunk*)csd->vram_start;
    csd->vmem = AllocVec(sizeof(struct MemHeader), MEMF_CLEAR | MEMF_PUBLIC);
    csd->vmem->mh_Node.ln_Type = NT_MEMORY;
    csd->vmem->mh_First = mc;
    csd->vmem->mh_Lower = (APTR)mc;
    csd->vmem->mh_Upper = (APTR)((ULONG)mc + size);
    csd->vmem->mh_Free = size;
    mc->mc_Next = NULL;
    mc->mc_Bytes = size;

    __IHidd_BitMap  	= OOP_ObtainAttrBase(IID_Hidd_BitMap);
    __IHidd_UAEGFXBitmap= OOP_ObtainAttrBase(IID_Hidd_UAEGFXBitMap);
    __IHidd_GC      	= OOP_ObtainAttrBase(IID_Hidd_GC);
    __IHidd_Sync    	= OOP_ObtainAttrBase(IID_Hidd_Sync);
    __IHidd_Gfx     	= OOP_ObtainAttrBase(IID_Hidd_Gfx);
    __IHidd_PixFmt	= OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    __IHidd_ColorMap 	= OOP_ObtainAttrBase(IID_Hidd_ColorMap);
    
    if (!__IHidd_BitMap || !__IHidd_UAEGFXBitmap || !__IHidd_GC ||
    	!__IHidd_Sync || !__IHidd_Gfx || !__IHidd_PixFmt || !__IHidd_ColorMap)
    {
    	D(bug("Init_UAEGFXClass fail\n"));
    	freeattrbases(csd);
    	return FALSE;
    }
    return TRUE;
}


static int Expunge_UAEGFXClass(LIBBASETYPEPTR LIBBASE)
{
    struct uaegfx_staticdata *csd = &LIBBASE->csd;
    D(bug("Expunge_UAEGFXClass\n"));
    FreeVec(csd->boardinfo);
    freeattrbases(csd);
    return TRUE;
}

ADD2EXPUNGELIB(Expunge_UAEGFXClass, 1)

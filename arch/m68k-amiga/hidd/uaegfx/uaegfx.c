/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English.
*/

#include <exec/libraries.h>
#include <exec/rawfmt.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <graphics/displayinfo.h>
#include <intuition/intuitionbase.h>
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

#include LC_LIBDEFS_FILE

#include "uaegfx.h"
#include "uaegfxbitmap.h"
#include "uaertg.h"

#define SDEBUG 0
#define DEBUG 0
#define DRTG(x) x;
#include <aros/debug.h>

#define SIZE_RESLIST 5
#define SIZE_PFLIST 19
#define SIZE_MODELIST (5 + RGBFB_MaxFormats)

HIDDT_ModeID *UAEGFXCl__Hidd_Gfx__QueryModeIDs(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_QueryModeIDs *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct RTGMode *node;
    struct TagItem *tag, *tstate;
    ULONG minwidth = 0, maxwidth = 0xFFFFFFFF;
    ULONG minheight = 0, maxheight = 0xFFFFFFFF;
    OOP_Object **pf = NULL;
    HIDDT_ModeID *modeids;
    WORD cnt;

   if (csd->superforward)
   	return (HIDDT_ModeID*)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    for (tstate = msg->queryTags; (tag = NextTagItem(&tstate)); )
    {
	switch (tag->ti_Tag)
	{
	    case tHidd_GfxMode_MinWidth:
	    	minwidth = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MaxWidth:
	    	maxwidth = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MinHeight:
	    	minheight = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MaxHeight:
	    	maxheight = (ULONG)tag->ti_Tag;
		break;
		
	    case tHidd_GfxMode_PixFmts:
	    	pf = (OOP_Object**)tag->ti_Tag;
		break;

	}
    }
    DB2(bug("QueryModeIDs (%dx%d)-(%dx%d) %p\n", minwidth, minheight, maxwidth, maxheight, pf));
    cnt = 0;
    ForeachNode(&csd->rtglist, node) {
	if (node->width >= minwidth && node->width <= maxwidth && node->height >= minheight && node->height <= maxheight) {
	    OOP_Object **pfp = NULL;
	    if (pf) {
	    	pfp = pf;
	    	while (*pfp) {
	    	    if (*pfp == node->pf) {
	    	    	pfp = NULL;
	    	    	break;
	    	    }
	    	    pfp++;
	    	}
	    }
	    if (!pfp)
	    	cnt++;
	}
    }
    modeids = AllocVec((cnt + 1) * sizeof(HIDDT_ModeID), MEMF_PUBLIC);
    if (!modeids)
    	return NULL;
    cnt = 0;
    ForeachNode(&csd->rtglist, node) {
 	if (node->width >= minwidth && node->width <= maxwidth && node->height >= minheight && node->height <= maxheight) {
	    OOP_Object **pfp = NULL;
	    if (pf) {
	    	pfp = pf;
	    	while (*pfp) {
	    	    if (*pfp == node->pf) {
	    	    	pfp = NULL;
	    	    	break;
	    	    }
	    	    pfp++;
	    	}
	    }
	    if (!pfp) {
    	    	DB2(bug("%d: %08x\n", cnt, node->modeid));
	    	modeids[cnt++] = node->modeid;
	    }
	}
    }
    modeids[cnt] = vHidd_ModeID_Invalid;
    return modeids;
}

VOID UAEGFXCl__Hidd_Gfx__ReleaseModeIDs(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ReleaseModeIDs *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    if (csd->superforward)
   	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    else
    	FreeVec(msg->modeIDs);
}

HIDDT_ModeID UAEGFXCl__Hidd_Gfx__NextModeID(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NextModeID *msg)
{
	struct uaegfx_staticdata *csd = CSD(cl);
	struct RTGMode *node = NULL;
	HIDDT_ModeID mid = vHidd_ModeID_Invalid;

    	DB2(bug("NextModeID %08x\n", msg->modeID));
	if (msg->modeID != vHidd_ModeID_Invalid) {
		ForeachNode(&csd->rtglist, node) {
			if (node->modeid == msg->modeID) {
				node = (struct RTGMode*)node->node.ln_Succ;
				break;
			}
		}
	}
	if (!node)
		node = (struct RTGMode*)csd->rtglist.lh_Head;
	if (node->node.ln_Succ) {
		mid = node->modeid;
		*msg->syncPtr = node->sync;
		*msg->pixFmtPtr = node->pf;
	}
	DB2(bug("=%08x %p %p\n", mid, *msg->syncPtr, *msg->pixFmtPtr));
	return mid;
}

BOOL UAEGFXCl__Hidd_Gfx__GetMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_GetMode *msg)
{
	struct uaegfx_staticdata *csd = CSD(cl);
	struct RTGMode *node;

 	if (csd->superforward)
   		return (BOOL)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

        DB2(bug("GetMode %08x\n", msg->modeID));
	ForeachNode(&csd->rtglist, node) {
		if (node->modeid == msg->modeID) {
			*msg->syncPtr = node->sync;
			*msg->pixFmtPtr = node->pf;
			DB2(bug("= %p %p\n", node->sync, node->pf));
			return TRUE;
		}
	}
	DB2(bug("= FAIL\n"));
	return FALSE;
}

struct RTGFormat
{
    UWORD rgbformat;
    ULONG rm, gm, bm, am;
    UWORD rs, gs, bs, as;
    BOOL endianswap;
};

static const struct RTGFormat formats[] =
{
    { RGBFB_CLUT,	0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000,  8, 16, 24,  0, FALSE },

    { RGBFB_B8G8R8A8,	0x0000ff00, 0x00ff0000, 0xff000000, 0x000000ff, 16,  8,  0, 24, FALSE },
    { RGBFB_R8G8B8A8,	0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff,  0,  8, 16, 24, FALSE },
    { RGBFB_A8B8G8R8,	0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, 24, 16,  8,  0, FALSE },
    { RGBFB_A8R8G8B8,	0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000,  8, 16, 24,  0, FALSE },

    { RGBFB_B8G8R8,	0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000, 24, 16,  8,  0, FALSE },
    { RGBFB_R8G8B8,	0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000,  8, 16, 24,  0, FALSE },

    { RGBFB_R5G5B5PC,	0x00007c00, 0x000003e0, 0x0000001f, 0x00000000, 17, 22, 27,  0, TRUE },
    { RGBFB_R5G6B5PC,	0x0000f800, 0x000007e0, 0x0000001f, 0x00000000, 16, 21, 27,  0, TRUE },
    { RGBFB_R5G5B5,	0x00007c00, 0x000003e0, 0x0000001f, 0x00000000, 17, 22, 27,  0, FALSE },
    { RGBFB_R5G6B5,	0x0000f800, 0x000007e0, 0x0000001f, 0x00000000, 16, 21, 27,  0, FALSE },
/*
    { RGBFB_B5G5R5PC,	0x0000003e, 0x000007c0, 0x0000f800, 0x00000000, 26, 21, 16,  0, TRUE },
    { RGBFB_B5G6R5PC,	0x0000001f, 0x000007e0, 0x0000f800, 0x00000000, 27, 21, 16,  0, TRUE },
*/
    { 0 }
};

static const UBYTE rgbtypelist[] = {
    RGBFB_CLUT,
    
    RGBFB_R5G6B5PC,
    RGBFB_R5G5B5PC,
    RGBFB_B5G6R5PC,
    RGBFB_B5G5R5PC,
    RGBFB_R5G6B5,
    RGBFB_R5G5B5,
    
    RGBFB_B8G8R8,
    RGBFB_R8G8B8,
    
    RGBFB_B8G8R8A8,
    RGBFB_A8B8G8R8,
    RGBFB_A8R8G8B8,
    RGBFB_R8G8B8A8,
    0
};

OOP_Object *UAEGFXCl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct Library *OOPBase = csd->cs_OOPBase;
    struct LibResolution *r;
    WORD rescnt, i, j, k, l;
    struct TagItem *reslist, *restags, *pflist, *modetags;
    struct pRoot_New mymsg;
    struct TagItem mytags[2];
    UWORD supportedformats, gotmodes;

    if (csd->initialized)
	return NULL;

    NEWLIST(&csd->rtglist);
    NEWLIST(&csd->bitmaplist);
    supportedformats = gw(csd->boardinfo + PSSO_BoardInfo_RGBFormats);
    rescnt = 0;
    ForeachNode(csd->boardinfo + PSSO_BoardInfo_ResolutionsList, r) {
        rescnt++;
    }
    D(bug("UAEGFX: resolutions: %d, supportmask: %x\n", rescnt, supportedformats));

    reslist = AllocVec(rescnt * SIZE_RESLIST * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);
    restags = AllocVec((rescnt + 1) * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);
    pflist = AllocVec(RGBFB_MaxFormats * SIZE_PFLIST * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);
    modetags = AllocVec(SIZE_MODELIST * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);
    
    i = 0;
    ForeachNode(csd->boardinfo + PSSO_BoardInfo_ResolutionsList, r) {
    	reslist[i * SIZE_RESLIST + 0].ti_Tag = aHidd_Sync_HDisp;
    	reslist[i * SIZE_RESLIST + 0].ti_Data = r->Width;
    	reslist[i * SIZE_RESLIST + 1].ti_Tag = aHidd_Sync_VDisp;
    	reslist[i * SIZE_RESLIST + 1].ti_Data = r->Height;
    	reslist[i * SIZE_RESLIST + 2].ti_Tag = aHidd_Sync_Description;
    	reslist[i * SIZE_RESLIST + 2].ti_Data = (IPTR)(csd->CardBase ? "RTGFX:%hx%v" : "UAEGFX:%hx%v");
    	reslist[i * SIZE_RESLIST + 3].ti_Tag = aHidd_Sync_PixelClock;
    	reslist[i * SIZE_RESLIST + 3].ti_Data = r->Modes[CHUNKY]->PixelClock;
    	reslist[i * SIZE_RESLIST + 4].ti_Tag = TAG_DONE;
    	reslist[i * SIZE_RESLIST + 4].ti_Data = 0;
    	D(bug("%08x %d*%d\n", r, r->Width, r->Height));
    	restags[i].ti_Tag = aHidd_Gfx_SyncTags;
    	restags[i].ti_Data = (IPTR)&reslist[i * SIZE_RESLIST];
    	i++;
    }
    restags[i].ti_Tag = TAG_DONE;
    restags[i].ti_Data = 0;
    
    gotmodes = 0;
    k = 0;
    j = 0;
    for (i = 0; rgbtypelist[i]; i++) {
   	UBYTE rgbtype = rgbtypelist[i];
   	WORD depth = getrtgdepth(1 << rgbtype);
    	if (!((1 << rgbtype) & RGBFB_SUPPORTMASK) || depth == 0 || !((1 << rgbtype) & supportedformats)) {
      	    pflist[j].ti_Tag = TAG_DONE;
     	    pflist[j].ti_Data = 0;
    	    j++;
    	    continue;
    	}
    	for (l = 0; formats[l].rgbformat; l++) {
    	    if (formats[l].rgbformat == rgbtype)
    	    	break;
    	}
    	if (formats[l].rgbformat == 0) {
       	    pflist[j].ti_Tag = TAG_DONE;
     	    pflist[j].ti_Data = 0;
    	    j++;
    	    continue;
    	}
   	D(bug("RTGFORMAT=%d found. Depth=%d\n", rgbtype, depth));
   	
   	if (gotmodes & (1 << (depth / 8))) {
	    D(bug("-> skipped\n"));
       	    pflist[j].ti_Tag = TAG_DONE;
     	    pflist[j].ti_Data = 0;
    	    j++;
    	    continue;
    	}

	gotmodes |= 1 << (depth / 8);

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
     	pflist[j].ti_Data = formats[l].as;
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
     	pflist[j].ti_Data = formats[l].am;
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
    	pflist[j].ti_Tag = aoHidd_PixFmt_SwapPixelBytes;
     	pflist[j].ti_Data = formats[l].endianswap;
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
	HIDDT_ModeID *midp;

	D(bug("UAEGFX::New(): Got object from super\n"));
	NewList((struct List *)&data->bitmaps);
	csd->initialized = 1;
	csd->spritecolors = 16;

	csd->superforward = TRUE;
    	midp = HIDD_Gfx_QueryModeIDs(o, NULL);
	for (i = 0; midp[i] != vHidd_ModeID_Invalid; i++) {
	    OOP_Object *sync, *pf;
	    HIDDT_ModeID mid = midp[i];
	    IPTR dwidth, dheight;
	    struct RTGMode *node1, *node2;
	    ULONG modeid, rtgmodeid, p96mode;
	    
	    if (!HIDD_Gfx_GetMode(o, mid, &sync, &pf))
	    	continue;
	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &dwidth);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &dheight);

	    DB2(bug("w=%d h=%d mode=%08x sync=%x pf=%x\n", dwidth, dheight, mid, sync, pf));

	    modeid = vHidd_ModeID_Invalid;
   	    ForeachNode(csd->boardinfo + PSSO_BoardInfo_ResolutionsList, r) {
	    	if (r->Width == dwidth && r->Height == dheight) {
	    	    modeid = r->DisplayID;
	    	    break;
	    	}
	    }
	    if (modeid == vHidd_ModeID_Invalid) {
	    	D(bug("w=%d h=%d not found!\n", dwidth, dheight));
	    	continue;
	    }

	    p96mode = getrtgformat(csd, pf);
	    rtgmodeid = (modeid & 0x00ff0000) | 0x1000 | (p96mode << 8);

	    ForeachNode(&csd->rtglist, node2) {
	    	if (node2->width == dwidth && node2->height == dheight && node2->modeid == rtgmodeid)
	    	    break;
	    }
	    if (node2->node.ln_Succ != NULL) {
	    	D(bug("w=%d h=%d mode=%08x already found!\n", dwidth, dheight, rtgmodeid));
	    	continue;
	    }

	    node1 = AllocMem(sizeof(struct RTGMode), MEMF_CLEAR);
	    node1->width = dwidth;
	    node1->height = dheight;
	    node1->pf = pf;
	    node1->sync = sync;
	    node1->modeid = rtgmodeid;
	    AddTail(&csd->rtglist, &node1->node);

	    DB2(bug("Added %dx%d %08x %d\n", node1->width, node1->height, node1->modeid, p96mode));
	}
	HIDD_Gfx_ReleaseModeIDs(o, midp);
	csd->superforward = FALSE;

    }
    
    FreeVec(restags);
    FreeVec(reslist);
    FreeVec(pflist);
    FreeVec(modetags);
    
    ReturnPtr("UAEGFX::New", OOP_Object *, o);
}

/********** GfxHidd::Dispose()  ******************************/
OOP_Object *UAEGFXCl__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{  
    struct uaegfx_staticdata *csd = CSD(cl);
    HIDDT_ModeID		modeid;
    struct pHidd_Gfx_NewBitMap   newbitmap;
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
    struct TagItem  *tag, *tstate;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
    	ULONG idx;
	D(bug("UAEGFXCl__Root__Set %x\n", tag->ti_Tag));
    	if (IS_GFX_ATTR(tag->ti_Tag, idx)) {
    	    D(bug("->%d\n", idx));
	    switch(idx)
	    {
	    case aoHidd_Gfx_ActiveCallBack:
	        csd->acb = (void *)tag->ti_Data;
		break;

	    case aoHidd_Gfx_ActiveCallBackData:
	        csd->acbdata = (APTR)tag->ti_Data;
		break;
	    }
    	}
    }
    OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
}
#if 0
ULONG UAEGFXCl__Hidd_Gfx__MakeViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_MakeViewPort *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);

    csd->vpe = NULL;
    if (!msg)
    	return MVP_OK;
    bug("makeviewport %p\n", msg->Data->vpe);
    csd->vpe = msg->Data->vpe;
    return MVP_OK;
}

void UAEGFXCl__Hidd_Gfx__CleanViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CleanViewPort *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);

    csd->vpe = NULL;
    bug("cleanviewport\n");
}
#endif

static void doshow(struct uaegfx_staticdata *csd, OOP_Object *bm, struct ViewPort *vp, BOOL offonly)
{
    struct IntuitionBase *ib = (struct IntuitionBase*)csd->cs_IntuitionBase;
    struct Library *OOPBase = csd->cs_OOPBase;
    struct ViewPort *vpi = NULL;

    if (ib->FirstScreen)
    	vpi = &ib->FirstScreen->ViewPort;

    D(bug("doshow b=%p vp=%p vpi=%p acb=%p acbd=%p\n", bm, vp, vpi, csd->acb, csd->acbdata));

    if (bm && vpi == vp) {
    	/* we are topmost screen -> show our display */
    	IPTR tags[] = {aHidd_BitMap_Visible, TRUE, TAG_DONE};

    	if (offonly)
    	    return;

        OOP_SetAttrs(bm, (struct TagItem *)tags);

	if (csd->acb)
	    csd->acb(csd->acbdata, NULL);

    } else if (bm) {
    	/* we are not topmost -> turn off our display */
   	IPTR tags[] = {aHidd_BitMap_Visible, FALSE, TAG_DONE};
        OOP_SetAttrs(bm, (struct TagItem *)tags);
    } else {
    	/* no display */
    	SetDisplay(csd, FALSE);
    	SetSwitch(csd, FALSE);
    }
}

OOP_Object *UAEGFXCl__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *c, struct pHidd_Gfx_Show *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);

    doshow(csd, msg->bitMap, csd->viewport, FALSE);
    return msg->bitMap;
}

ULONG UAEGFXCl__Hidd_Gfx__PrepareViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ShowViewPorts *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct HIDD_ViewPortData *vpd = msg->Data;
    OOP_Object *bm = NULL;
    struct ViewPort *vp = NULL;

    if (vpd) {
    	bm = vpd->Bitmap;
    	if (vpd->vpe)
    	    vp = vpd->vpe->ViewPort;
    }
    csd->viewport = vp;
    doshow(csd, bm, vp, FALSE);

    bug("PrepareViewPorts viewport=%p\n", csd->viewport);
    return MCOP_OK;
}

ULONG UAEGFXCl__Hidd_Gfx__ShowViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ShowViewPorts *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct HIDD_ViewPortData *vpd = msg->Data;
    OOP_Object *bm = NULL;
    struct ViewPort *vp = NULL;

    if (vpd) {
    	bm = vpd->Bitmap;
    	if (vpd->vpe)
    	    vp = vpd->vpe->ViewPort;
    }
    doshow(csd, bm, vp, FALSE);
    return TRUE;
}

VOID UAEGFXCl__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    struct bm_data *sdata = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
    struct bm_data *ddata = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
    struct RenderInfo risrc, ridst;

    WaitBlitter(csd);
    if (sdata->rgbformat != ddata->rgbformat || !sdata->invram || !ddata->invram) {
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

BOOL UAEGFXCl__Hidd_Gfx__CopyBoxMasked(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBoxMasked *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);

    WaitBlitter(csd);
    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL UAEGFXCl__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *shape, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    OOP_Object *cm = NULL;
    IPTR width, height;
    WORD x, y, hiressprite, i;
    UWORD *p;
    ULONG flags;
    struct Library *OOPBase = csd->cs_OOPBase;

    OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);
    OOP_GetAttr(msg->shape, aHidd_BitMap_ColorMap, (IPTR*)&cm);
    if (cm) {
	for (i = 0; i < 3; i++) {
	    HIDDT_Color c;
	    HIDD_CM_GetColor(cm, i + 1, &c);
	    SetSpriteColor(csd, i, c.red, c.green, c.blue);
	}
    }
    Forbid();
    pb(csd->boardinfo + PSSO_BoardInfo_MouseXOffset, msg->xoffset);
    pb(csd->boardinfo + PSSO_BoardInfo_MouseYOffset, msg->yoffset);
    p = (UWORD*)gp(csd->boardinfo + PSSO_BoardInfo_MouseImage);
    if (p == NULL || width != csd->sprite_width || height != csd->sprite_height) {
    	FreeVec(p);
    	p = AllocVec(4 + 4 + ((width + 15) & ~15) / 8 * height * 2, MEMF_CLEAR | MEMF_PUBLIC);
        pp(csd->boardinfo + PSSO_BoardInfo_MouseImage, p);
    	if (!p) {
    	    Permit();
    	    return FALSE;
    	}
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
    pw(csd->boardinfo + PSSO_BoardInfo_MouseX, msg->x + (BYTE)csd->boardinfo[PSSO_BoardInfo_MouseXOffset]);
    pw(csd->boardinfo + PSSO_BoardInfo_MouseY, msg->y + (BYTE)csd->boardinfo[PSSO_BoardInfo_MouseYOffset]);
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
    struct Library *OOPBase = csd->cs_OOPBase;
    IPTR width, height, bpp;
    
    OOP_GetAttr(msg->sync, aHidd_Sync_HDisp, &width);
    OOP_GetAttr(msg->sync, aHidd_Sync_VDisp, &height);
    OOP_GetAttr(msg->pixFmt, aHidd_PixFmt_BytesPerPixel, &bpp);
    if (width > csd->maxwidth[bpp])
	return FALSE;
    if (height > csd->maxheight[bpp])
	return FALSE;
    return width * height * bpp < csd->vram_size;
}


static void freeattrbases(LIBBASETYPEPTR LIBBASE, struct uaegfx_staticdata *csd)
{
    struct Library *OOPBase = GM_OOPBASE_FIELD(LIBBASE);

    OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    OOP_ReleaseAttrBase(IID_Hidd_UAEGFXBitMap);
    OOP_ReleaseAttrBase(IID_Hidd_GC);
    OOP_ReleaseAttrBase(IID_Hidd_Sync);
    OOP_ReleaseAttrBase(IID_Hidd_Gfx);
    OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
    OOP_ReleaseAttrBase(IID_Hidd_ColorMap);
}

AROS_UFIH1(rtg_vblank, APTR, boardinfo)
{
    AROS_USERFUNC_INIT

    (void)boardinfo;
    return 0;	

    AROS_USERFUNC_EXIT

}

struct P96RTGmode
{
    UWORD w, h, id;
    ULONG clock;
    UWORD htotal, vtotal;
    UWORD hborder, vborder;
    UWORD hpos, vpos;
    UWORD hsync, vsync;
    UBYTE flags;
};

static const struct P96RTGmode rtgmodes[] =
{
	{  320, 240, 1, 16000000,  416, 260, 0, 0, 40,  5,  16, 1, GMF_HPOLARITY | GMF_VPOLARITY | GMF_DOUBLESCAN },
	{  640, 480, 3, 31000000,  832, 520, 0, 0, 48,  9,  80, 3, GMF_HPOLARITY | GMF_VPOLARITY },
	{  800, 600, 4, 40100000, 1056, 620, 0, 0, 56,  1, 112, 2, 0 },
	{ 1024, 768, 5, 65000000, 1344, 806, 0, 0, 88,  3,  88, 6, GMF_HPOLARITY | GMF_VPOLARITY },
	{ 0 }
};
/* real RTG only */
static BOOL PopulateModeInfo(struct uaegfx_staticdata *csd, struct LibResolution *res, const struct P96RTGmode *mode)
{
    UWORD rgbformat;
    BOOL ok = FALSE;

    for (rgbformat = 0; rgbformat < RGBFB_MaxFormats; rgbformat++) {
    	ULONG clockindex;
    	UBYTE depth, index;
    	struct ModeInfo *mi;
    	UWORD maxhval, maxvval, maxhres, maxvres;

    	if (!((1 << rgbformat) & RGBFB_SUPPORTMASK))
    	    continue;
    	if (!((1 << rgbformat) & gw(csd->boardinfo + PSSO_BoardInfo_RGBFormats)))
    	    continue;
    	depth = getrtgdepth(1 << rgbformat);
    	index = (depth + 7) / 8;
    	if (res->Modes[index])
    	    continue;

     	maxhval = gw(csd->boardinfo + PSSO_BoardInfo_MaxHorValue + index * 2);
     	maxvval = gw(csd->boardinfo + PSSO_BoardInfo_MaxVerValue + index * 2);
     	maxhres = gw(csd->boardinfo + PSSO_BoardInfo_MaxHorResolution + index * 2);
     	maxvres = gw(csd->boardinfo + PSSO_BoardInfo_MaxVerResolution + index * 2);

    	if (mode->htotal > maxhval || mode->vtotal > maxvval ||
    	    mode->w > maxhres || mode->h > maxvres)
    	    continue;

    	mi = AllocMem(sizeof(struct ModeInfo), MEMF_CLEAR | MEMF_PUBLIC);
    	if (!mi)
    	    continue;
	mi->OpenCount = 1;
	mi->Active = TRUE;
	mi->Width = mode->w;
	mi->Height = mode->h;
	mi->Depth = depth;
	mi->HorTotal = mode->htotal;
	mi->VerTotal = mode->vtotal;
	mi->HorBlankSize = mode->hborder;
	mi->VerBlankSize = mode->vborder;
	mi->HorSyncStart = mode->hpos;
	mi->VerSyncStart = mode->vpos;
	mi->HorSyncSize = mode->hsync;
	mi->VerSyncSize = mode->vsync;
	mi->Flags = mode->flags;
        clockindex = ResolvePixelClock(csd, mi, mode->clock, rgbformat);
        mi->PixelClock = GetPixelClock(csd, mi, clockindex, rgbformat);
        DRTG(bug("%d,%p: %dx%dx%d ci=%d clk=%d (%d/%d)\n",
            index, mi, mi->Width, mi->Height, mi->Depth,
            clockindex, mi->PixelClock, mi->Numerator, mi->Denominator));
        res->Modes[index] = mi;
        ok = TRUE;
    }
    return ok;
}
static void PopulateResolutionList(struct uaegfx_staticdata *csd)
{
    struct LibResolution *node;
    UWORD cnt;
    
    NEWLIST((csd->boardinfo + PSSO_BoardInfo_ResolutionsList));
    for (cnt = 0; rtgmodes[cnt].clock; cnt++) {
    	const struct P96RTGmode *mode = &rtgmodes[cnt];
        node = AllocMem(sizeof(struct LibResolution), MEMF_CLEAR | MEMF_PUBLIC);
        if (!node)
            break;
        node->Width = mode->w;
        node->Height = mode->h;
        node->DisplayID = 0x50001000 | (mode->id << 16);
        node->BoardInfo = csd->boardinfo;
        if (PopulateModeInfo(csd, node, mode))
	    AddTail((struct List*)(csd->boardinfo + PSSO_BoardInfo_ResolutionsList), (struct Node*)node);
	else
	    FreeMem(node, sizeof(struct LibResolution));
    }   
}

static int openall(struct uaegfx_staticdata *csd)
{
    if ((csd->cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY))) {
    	if ((csd->cs_IntuitionBase = TaggedOpenLibrary(TAGGEDOPEN_INTUITION))) {
    	    if ((csd->cs_OOPBase = OpenLibrary("oop.library", 0))) {
    	        return TRUE;
            }
    	}
    }
    return FALSE;
}
static void freeall(struct uaegfx_staticdata *csd)
{
    CloseLibrary(csd->cs_IntuitionBase);
    CloseLibrary(csd->cs_UtilityBase);
    CloseLibrary(csd->cs_OOPBase);
    FreeMem(csd->vmem, sizeof(struct MemHeader));
    csd->vmem = NULL;
    FreeVec(csd->boardinfo);
    csd->boardinfo = NULL;
}

static void P96DebugInfo(struct uaegfx_staticdata *csd)
{
    UBYTE i;
    DRTG(bug("Name:'%s'\n",
	gl(csd->boardinfo + PSSO_BoardInfo_BoardName)));
    DRTG(bug("Reg:%08x IO:%08x\n",
	gl(csd->boardinfo + PSSO_BoardInfo_RegisterBase),
	gl(csd->boardinfo + PSSO_BoardInfo_MemoryIOBase)));
    DRTG(bug("BoardType:%d GCType:%d PCType:%d BPC:%d Flags:%08x\n",
	gl(csd->boardinfo + PSSO_BoardInfo_BoardType),
	gl(csd->boardinfo + PSSO_BoardInfo_GraphicsControllerType),
	gl(csd->boardinfo + PSSO_BoardInfo_PaletteChipType),
	gw(csd->boardinfo + PSSO_BoardInfo_BitsPerCannon),
	gl(csd->boardinfo + PSSO_BoardInfo_Flags)));
    for (i = 0; i < MAXMODES; i++) {
    	DRTG(bug("%d: HV:%4d VV: %4d HR:%4d VR:%4d C:%d\n", i,
    	    gw(csd->boardinfo + PSSO_BoardInfo_MaxHorValue + i * 2),
    	    gw(csd->boardinfo + PSSO_BoardInfo_MaxVerValue + i * 2),
    	    gw(csd->boardinfo + PSSO_BoardInfo_MaxHorResolution + i * 2),
    	    gw(csd->boardinfo + PSSO_BoardInfo_MaxVerResolution + i * 2),
    	    gl(csd->boardinfo + PSSO_BoardInfo_PixelClockCount + i * 4)));
    }
}

static BOOL P96Init(struct uaegfx_staticdata *csd, struct Library *lib)
{
    DRTG(bug("P96GFX: attempting to init '%s'\n", lib->lib_Node.ln_Name));
    pl(csd->boardinfo + PSSO_BoardInfo_CardBase, (ULONG)lib);
    csd->CardBase = lib;
    InitRTG(csd->boardinfo);
    if (FindCard(csd)) {
	DRTG(bug("P96GFX: FindCard succeeded\n"));
	if (InitCard(csd)) {
	    DRTG(bug("P96GFX: InitCard succeeded\n"));
	    SetInterrupt(csd, FALSE);
	    /* Without this card may not be in linear memory map mode. */
	    SetMemoryMode(csd, RGBFB_CLUT);
 	    P96DebugInfo(csd);
	    return TRUE;
	}
    }
    pl(csd->boardinfo + PSSO_BoardInfo_CardBase, 0);
    csd->CardBase = NULL;
    return FALSE;
}


BOOL Init_UAEGFXClass(LIBBASETYPEPTR LIBBASE)
{
    struct uaegfx_staticdata *csd = &LIBBASE->csd;
    struct MemChunk *mc;
    struct Library *OOPBase;
    UBYTE i;
    struct Interrupt *intr;
    struct Node *node;

    if (!(SysBase->AttnFlags & AFF_68020))
    	return FALSE;

    D(bug("Init_UAEGFXClass\n"));
    if (!openall(csd)) {
    	freeall(csd);
    	return FALSE;
    }

    OOPBase = csd->cs_OOPBase;

    csd->boardinfo = AllocVec(PSSO_BoardInfo_SizeOf + PSSO_BitMapExtra_Last + sizeof(struct ModeInfo), MEMF_CLEAR | MEMF_PUBLIC);
    if (!csd->boardinfo) {
    	freeall(csd);
    	return FALSE;
    }
    NEWLIST((struct List*)(csd->boardinfo + PSSO_BoardInfo_ResolutionsList));
    NEWLIST((struct List*)(csd->boardinfo + PSSO_BoardInfo_BitMapList));
    NEWLIST((struct List*)(csd->boardinfo + PSSO_BoardInfo_MemList));
    NEWLIST((struct List*)(csd->boardinfo + PSSO_BoardInfo_WaitQ));
    csd->bitmapextra = csd->boardinfo + PSSO_BoardInfo_SizeOf;
    csd->fakemodeinfo = (struct ModeInfo*)(csd->boardinfo + PSSO_BoardInfo_SizeOf + PSSO_BitMapExtra_Last);
    pl(csd->boardinfo + PSSO_BoardInfo_BitMapExtra, (ULONG)csd->bitmapextra);
    pl(csd->boardinfo + PSSO_BoardInfo_ExecBase, (ULONG)SysBase);
    pl(csd->boardinfo + PSSO_BoardInfo_UtilBase, (ULONG)csd->cs_UtilityBase);
    InitSemaphore((struct SignalSemaphore*)(csd->boardinfo + PSSO_BoardInfo_BoardLock));
    intr = (struct Interrupt*)(csd->boardinfo + PSSO_BoardInfo_HardInterrupt);
    intr->is_Code = (APTR)rtg_vblank;
    intr->is_Data         = csd->boardinfo;
    intr->is_Node.ln_Name = "RTG VBlank";
    intr->is_Node.ln_Pri  = 0;
    intr->is_Node.ln_Type = NT_INTERRUPT;
    intr = (struct Interrupt*)(csd->boardinfo + PSSO_BoardInfo_SoftInterrupt);
    intr->is_Code = (APTR)rtg_vblank;
    intr->is_Data         = csd->boardinfo;
    intr->is_Node.ln_Name = "RTG VBlank";
    intr->is_Node.ln_Pri  = 0;
    intr->is_Node.ln_Type = NT_INTERRUPT;

    Forbid();
    ForeachNode(&SysBase->LibList.lh_Head, node) {
   	struct Library *lib = (struct Library*)node;
    	char *name = node->ln_Name;
    	int len = strlen(name);
    	if (len > 5 && !stricmp(name + len - 5, ".card")) {
	    BOOL ret;
	    Permit();
	    ret = P96Init(csd, lib);
	    Forbid();
	    if (ret)
    	    	break;
	    DRTG(bug("P96GFX: init failed\n"));   
	}
    }	
    Permit();

    if (!csd->CardBase) {
    	csd->uaeromvector = (APTR)(0xf00000 + 0xff60);
    	if ((gl(csd->uaeromvector) & 0xff00ffff) != 0xa0004e75) {
	    D(bug("UAE boot ROM entry point not found. UAEGFX not enabled.\n"));
	    freeall(csd);
	    return FALSE;
    	}
    	if (!FindCard(csd)) {
    	    D(bug("UAEGFX: FindCard() returned false\n"));
    	    freeall(csd);
    	    return FALSE;
        }
        D(bug("UAEGFX: FindCard done\n"));
        InitCard(csd);
	csd->hardwaresprite = (gl(csd->boardinfo + PSSO_BoardInfo_Flags) & (1 << BIB_HARDWARESPRITE)) && SetSprite(csd, FALSE);
    } else {
	PopulateResolutionList(csd);
	csd->hardwaresprite = gl(csd->boardinfo + PSSO_BoardInfo_Flags) & (1 << BIB_HARDWARESPRITE);
    }

    if (IsListEmpty((struct List*)(csd->boardinfo + PSSO_BoardInfo_ResolutionsList))) {
     	D(bug("Resolutionlist is empty, init failed.\n"));
     	freeall(csd);
    	return FALSE;
    }
    for (i = 0; i < MAXMODES; i++) {
    	csd->maxwidth[i] = gw(csd->boardinfo + PSSO_BoardInfo_MaxHorResolution + i * 2);
    	csd->maxheight[i] = gw(csd->boardinfo + PSSO_BoardInfo_MaxVerResolution + i * 2);
    }

    D(bug("InitCard done\n"));

    DRTG(bug("hardware sprite: %d\n", csd->hardwaresprite));

    csd->vram_start = (UBYTE*)gl(csd->boardinfo + PSSO_BoardInfo_MemoryBase);
    csd->vram_size = gl(csd->boardinfo + PSSO_BoardInfo_MemorySize);

    DRTG(bug("P96RTG VRAM found at %08x size %08x\n", csd->vram_start, csd->vram_size));
    mc = (struct MemChunk*)csd->vram_start;
    csd->vmem = AllocVec(sizeof(struct MemHeader), MEMF_CLEAR | MEMF_PUBLIC);
    csd->vmem->mh_Node.ln_Type = NT_MEMORY;
    csd->vmem->mh_First = mc;
    csd->vmem->mh_Lower = (APTR)mc;
    csd->vmem->mh_Upper = (APTR)((ULONG)mc + csd->vram_size);
    csd->vmem->mh_Free = csd->vram_size;
    mc->mc_Next = NULL;
    mc->mc_Bytes = csd->vmem->mh_Free;

    __IHidd_BitMap  	= OOP_ObtainAttrBase(IID_Hidd_BitMap);
    __IHidd_UAEGFXBitmap= OOP_ObtainAttrBase(IID_Hidd_UAEGFXBitMap);
    __IHidd_GC      	= OOP_ObtainAttrBase(IID_Hidd_GC);
    __IHidd_Sync    	= OOP_ObtainAttrBase(IID_Hidd_Sync);
    __IHidd_Gfx     	= OOP_ObtainAttrBase(IID_Hidd_Gfx);
    __IHidd_PixFmt	= OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    __IHidd_ColorMap 	= OOP_ObtainAttrBase(IID_Hidd_ColorMap);

    HiddBitMapBase	= OOP_GetMethodID(IID_Hidd_BitMap, 0);
    HiddColorMapBase	= OOP_GetMethodID(IID_Hidd_ColorMap, 0);
    HiddGfxBase		= OOP_GetMethodID(IID_Hidd_Gfx, 0);
    
    if (!__IHidd_BitMap || !__IHidd_UAEGFXBitmap || !__IHidd_GC ||
    	!__IHidd_Sync || !__IHidd_Gfx || !__IHidd_PixFmt || !__IHidd_ColorMap)
    {
    	D(bug("Init_UAEGFXClass fail\n"));
    	freeattrbases(LIBBASE, csd);
    	freeall(csd);
    	return FALSE;
    }

    DRTG(bug("P96RTG done\n"));
    return TRUE;
}

static int Expunge_UAEGFXClass(LIBBASETYPEPTR LIBBASE)
{
    struct uaegfx_staticdata *csd = &LIBBASE->csd;
    D(bug("Expunge_UAEGFXClass\n"));
    freeattrbases(LIBBASE, csd);
    freeall(csd);
    return TRUE;
}

ADD2EXPUNGELIB(Expunge_UAEGFXClass, 1)

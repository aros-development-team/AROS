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
#define DB2(x) ;
#include <aros/debug.h>

#define NATIVEMODES (3 * 4 * 1)
static const UWORD widthtable[] = { 320, 640, 1280, 0 };
static const UWORD heighttable[] = { 200, 256, 400, 512, 0 };

ULONG AmigaVideoCl__Hidd_Gfx__ModeProperties(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ModeProperties *msg)
{
    msg->props->DisplayInfoFlags = DIPF_IS_SPRITES | DIPF_IS_DRAGGABLE |
    	DIPF_IS_SPRITES_ATT | DIPF_IS_SPRITES_CHNG_BASE | DIPF_IS_SPRITES_CHNG_PRI |
    	DIPF_IS_DBUFFER;
    msg->props->NumHWSprites = 8;
    if ((msg->modeID & (PAL_MONITOR_ID | NTSC_MONITOR_ID)) == PAL_MONITOR_ID)
    	msg->props->DisplayInfoFlags |= DIPF_IS_PAL;
    if (msg->modeID & LORESLACE_KEY)
    	msg->props->DisplayInfoFlags |= DIPF_IS_LACE;
    if (msg->modeID & HAM_KEY)
    	msg->props->DisplayInfoFlags |= DIPF_IS_HAM;
    if (msg->modeID & EXTRAHALFBRITE_KEY)
    	msg->props->DisplayInfoFlags |= DIPF_IS_EXTRAHALFBRITE;
     return sizeof(struct HIDD_ModeProperties);
}

HIDDT_ModeID *AmigaVideoCl__Hidd_Gfx__QueryModeIDs(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_QueryModeIDs *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct NativeChipsetMode *node;
    struct TagItem *tag, *tstate;
    ULONG minwidth = 0, maxwidth = 0xFFFFFFFF;
    ULONG minheight = 0, maxheight = 0xFFFFFFFF;
    HIDDT_ModeID *modeids;
    WORD cnt;

   if (csd->superforward)
   	return (HIDDT_ModeID*)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    for (tstate = msg->queryTags; (tag = NextTagItem((const struct TagItem **)&tstate)); )
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
	    	/* all chipset modes have same pixelformat */
		break;

	}
    }
    DB2(bug("QueryModeIDs (%dx%d)-(%dx%d)\n", minwidth, minheight, maxwidth, maxheight));
    cnt = 0;
    ForeachNode(&csd->nativemodelist, node) {
	if (node->width >= minwidth && node->width <= maxwidth && node->height >= minheight && node->height <= maxheight) {
	    cnt++;
	}
    }
    modeids = AllocVec((cnt + 1) * sizeof(HIDDT_ModeID), MEMF_PUBLIC);
    if (!modeids)
    	return NULL;
    cnt = 0;
    ForeachNode(&csd->nativemodelist, node) {
 	if (node->width >= minwidth && node->width <= maxwidth && node->height >= minheight && node->height <= maxheight) {
    	    DB2(bug("%d: %08x\n", cnt, node->modeid));
	    modeids[cnt++] = node->modeid;
	}
    }
    modeids[cnt] = vHidd_ModeID_Invalid;
    return modeids;
}

VOID AmigaVideoCl__Hidd_Gfx__ReleaseModeIDs(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ReleaseModeIDs *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    if (csd->superforward)
   	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    else
    	FreeVec(msg->modeIDs);
}

HIDDT_ModeID AmigaVideoCl__Hidd_Gfx__NextModeID(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NextModeID *msg)
{
	struct amigavideo_staticdata *csd = CSD(cl);
	struct NativeChipsetMode *node = NULL;
	HIDDT_ModeID mid = vHidd_ModeID_Invalid;

    	DB2(bug("NextModeID %08x\n", msg->modeID));
	if (msg->modeID != vHidd_ModeID_Invalid) {
		ForeachNode(&csd->nativemodelist, node) {
			if (node->modeid == msg->modeID) {
				node = (struct NativeChipsetMode*)node->node.ln_Succ;
				break;
			}
		}
	}
	if (!node)
		node = (struct NativeChipsetMode*)csd->nativemodelist.lh_Head;
	if (node->node.ln_Succ) {
		mid = node->modeid;
		*msg->syncPtr = node->sync;
		*msg->pixFmtPtr = node->pf;
	}
	DB2(bug("=%08x %p %p\n", mid, *msg->syncPtr, *msg->pixFmtPtr));
	return mid;
}

BOOL AmigaVideoCl__Hidd_Gfx__GetMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_GetMode *msg)
{
	struct amigavideo_staticdata *csd = CSD(cl);
	struct NativeChipsetMode *node;

 	if (csd->superforward)
   		return (BOOL)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

        DB2(bug("GetMode %08x\n", msg->modeID));
	ForeachNode(&csd->nativemodelist, node) {
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

#define ADDTAG(tag,data) { *tagptr++ = tag; *tagptr++ = data; }


static void makemodename(ULONG modeid, UBYTE *bufptr)
{
    BOOL special = FALSE;
    
    special = (modeid & (HAM_KEY | EXTRAHALFBRITE_KEY | LORESDPF2_KEY)) != 0;
    strcpy(bufptr, (modeid & PAL_MONITOR_ID) == PAL_MONITOR_ID ? "PAL" : "NTSC");
    strcat(bufptr, ":");
    if ((modeid & (HIRES_KEY | SUPER_KEY)) == LORES_KEY)
    	strcat(bufptr, special ? "LowRes" : "Low Res");
    else if ((modeid & (HIRES_KEY | SUPER_KEY)) == HIRES_KEY)
    	strcat(bufptr, special ? "HighRes" : "High Res");
    else
    	strcat(bufptr, special ? "SuperHighRes" : "Super-High Res");
    if (modeid & HAM_KEY)
    	strcat(bufptr, " HAM");
    if (modeid & EXTRAHALFBRITE_KEY)
    	strcat(bufptr, " EHB");
    if ((modeid & LORESDPF2_KEY) == LORESDPF_KEY)
    	strcat(bufptr, " DualPF");
    if ((modeid & LORESDPF2_KEY) == LORESDPF2_KEY)
    	strcat(bufptr, " DualPF2");
    if (modeid & LORESLACE_KEY)
     	strcat(bufptr, special ? " Interlace" : " Laced");
    DB2(bug("%08x '%s'\n", modeid, bufptr));
}

static struct NativeChipsetMode *addmodeid(struct amigavideo_staticdata *csd, ULONG modeid, WORD w, WORD h)
{
    struct NativeChipsetMode *m;

    m = AllocMem(sizeof(struct NativeChipsetMode), MEMF_CLEAR | MEMF_PUBLIC);
    DB2(bug("%p %08x %dx%d\n", m, modeid, w, h));
    m->width = w;
    m->height = h;
    m->modeid = modeid;
    AddTail(&csd->nativemodelist, &m->node);
    return m;
}

OOP_Object *AmigaVideoCl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
	struct amigavideo_staticdata *csd = CSD(cl);
	struct TagItem mytags[2];
	struct pRoot_New mymsg;
	ULONG allocsize = 1200, allocsizebuf = 400;
	ULONG allocedsize = 0, allocedsizebuf = 0;
	WORD x, y, cnt, i;

	UBYTE *buf, *bufptr;
	ULONG *tags, *tagptr;
	ULONG *modetags[NATIVEMODES], modeids[NATIVEMODES];
	ULONG *pftags_aga, *pftags_ecs_shres, *pftags_ecs_hires, *pftags_ecs_lores;
	ULONG *mode_tags_aga, *mode_tags_ecs;

	if (csd->initialized)
		return NULL;

    	NewList(&csd->nativemodelist);
	tags = tagptr = AllocVec(allocsize, MEMF_PUBLIC | MEMF_REVERSE);
	buf = bufptr = AllocVec(allocsizebuf, MEMF_PUBLIC | MEMF_REVERSE);

	cnt = 0;
	for (y = 0; heighttable[y]; y++) {
		WORD h = heighttable[y];
		for (x = 0; widthtable[x]; x++) {
			WORD w = widthtable[x];
			ULONG modeid;
			
			modeid = 0;
			if (w == 1280) {
				modeid |= SUPER_KEY;
				if (!csd->aga && !csd->ecs_denise)
					continue;
			}
			else if (w == 640)
				modeid |= HIRES_KEY;
			if (h >= 400)
				modeid |= LORESLACE_KEY;
			if (h == 200 || h == 400)
				modeid |= NTSC_MONITOR_ID;
			else
				modeid |= PAL_MONITOR_ID;		

			addmodeid(csd, modeid, w, h);
    			modetags[cnt] = tagptr;
			modeids[cnt++] = modeid;

			ADDTAG(aHidd_Sync_HDisp, w);
			ADDTAG(aHidd_Sync_VDisp, h);
			ADDTAG(aHidd_Sync_Flags, h >= 400 ? vHidd_Sync_Interlaced : 0);
			ADDTAG(TAG_DONE, 0);
			
		}
	}

	if (csd->aga) {

		pftags_aga = tagptr;
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
		ADDTAG(aHidd_PixFmt_Depth,		8);
		ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
		ADDTAG(aHidd_PixFmt_BitsPerPixel,	8);
		ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
		ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
		ADDTAG(TAG_DONE, 0);
		
		mode_tags_aga = tagptr;
		ADDTAG(aHidd_Sync_HMin,		112);
		ADDTAG(aHidd_Sync_VMin,		112);
		ADDTAG(aHidd_Sync_HMax,		16384);
		ADDTAG(aHidd_Sync_VMax,		16384);
		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_aga);

		for (i = 0; i < cnt; i++) {
			makemodename(modeids[i], bufptr);
			ADDTAG(aHidd_Sync_Description,	(IPTR)bufptr);
			bufptr += strlen(bufptr) + 1;
			ADDTAG(aHidd_Gfx_SyncTags, (IPTR)modetags[i]);
		}

		ADDTAG(TAG_DONE, 0);

    		mytags[0].ti_Tag = aHidd_Gfx_ModeTags;
    		mytags[0].ti_Data = (IPTR)mode_tags_aga;

	} else {

		pftags_ecs_lores = tagptr;
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
		ADDTAG(aHidd_PixFmt_Depth,		6);
		ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
		ADDTAG(aHidd_PixFmt_BitsPerPixel,	6);
		ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
		ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
		ADDTAG(TAG_DONE, 0);

		pftags_ecs_hires = tagptr;
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
		ADDTAG(aHidd_PixFmt_Depth,		4);
		ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
		ADDTAG(aHidd_PixFmt_BitsPerPixel,	4);
		ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
		ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
		ADDTAG(TAG_DONE, 0);

		pftags_ecs_shres = tagptr;
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
		ADDTAG(aHidd_PixFmt_Depth,		2);
		ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
		ADDTAG(aHidd_PixFmt_BitsPerPixel,	2);
		ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
		ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
		ADDTAG(TAG_DONE, 0);

		mode_tags_ecs = tagptr;
		ADDTAG(aHidd_Sync_HMin,		112);
		ADDTAG(aHidd_Sync_VMin,		112);
		ADDTAG(aHidd_Sync_HMax,		1008);
		ADDTAG(aHidd_Sync_VMax,		1008);

		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_shres);
		for (i = 0; i < cnt; i++) {
			if ((modeids[i] & SUPER_KEY) != SUPER_KEY)
				continue;
			makemodename(modeids[i], bufptr);
			ADDTAG(aHidd_Sync_Description,	(IPTR)bufptr);
			bufptr += strlen(bufptr) + 1;
			ADDTAG(aHidd_Gfx_SyncTags, (IPTR)modetags[i]);
		}

		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_hires);
		for (i = 0; i < cnt; i++) {
			if ((modeids[i] & SUPER_KEY) != HIRES_KEY)
				continue;
			makemodename(modeids[i], bufptr);
			ADDTAG(aHidd_Sync_Description,	(IPTR)bufptr);
			bufptr += strlen(bufptr) + 1;
			ADDTAG(aHidd_Gfx_SyncTags, (IPTR)modetags[i]);
		}

		ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_lores);
		for (i = 0; i < cnt; i++) {
			if ((modeids[i] & SUPER_KEY) != LORES_KEY)
				continue;
			makemodename(modeids[i], bufptr);
			ADDTAG(aHidd_Sync_Description,	(IPTR)bufptr);
			bufptr += strlen(bufptr) + 1;
			ADDTAG(aHidd_Gfx_SyncTags, (IPTR)modetags[i]);
		}

		ADDTAG(TAG_DONE, 0);

     		mytags[0].ti_Tag = aHidd_Gfx_ModeTags;
    		mytags[0].ti_Data = (IPTR)mode_tags_ecs;
	}

    allocedsize = (ULONG)tagptr - (ULONG)tags;
    allocedsizebuf = bufptr - buf;
    D(bug("alloc=%d alloced=%d\n", allocsize, allocedsize));
    D(bug("allocbuf=%d allocedbuf=%d\n", allocsizebuf, allocedsizebuf));

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
	struct NativeChipsetMode *node;
	HIDDT_ModeID *midp;

	D(bug("AGFX::New(): Got object from super\n"));
	NewList((struct List *)&data->bitmaps);
	csd->initialized = 1;

	csd->superforward = TRUE;
    	midp = HIDD_Gfx_QueryModeIDs(o, NULL);
	for (i = 0; midp[i] != vHidd_ModeID_Invalid; i++) {
	    OOP_Object *sync, *pf;
	    HIDDT_ModeID mid = midp[i];
	    IPTR dwidth, dheight;
	    BOOL found = FALSE;
	    
	    DB2(bug("mid=%08x\n", mid));
	    if (!HIDD_Gfx_GetMode(o, mid, &sync, &pf))
	    	continue;
	    DB2(bug("sync=%x pf=%x\n", sync, pf));
	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &dwidth);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &dheight);
	    ForeachNode(&csd->nativemodelist, node) {
	    	if (node->width == dwidth && node->height == dheight) {
	    	    node->sync = sync;
	    	    node->pf = pf;
	    	    found = TRUE;
	    	    DB2(bug("%08x %dx%d sync = %p pf = %p\n", node->modeid, dwidth, dheight, sync, pf));
	    	}
	    }
	    if (!found)
	    	DB2(bug("%dx%d sync not found!\n", dwidth, dheight));
	}
	HIDD_Gfx_ReleaseModeIDs(o, midp);
	csd->superforward = FALSE;
	
    }
    FreeVec(buf);
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
    	IPTR modeid = vHidd_ModeID_Invalid;

	OOP_GetAttr(msg->bitMap, aHidd_BitMap_ModeID , &modeid);
	csd->modeid = modeid;
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

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for VMWareSVGA hidd.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <aros/symbolsets.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "bitmap.h"
#include "vmwaregfxclass.h"

#include LC_LIBDEFS_FILE

/* Don't initialize static variables with "=0", otherwise they go into DATA segment */

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddVMWareGfxAttrBase;
static OOP_AttrBase HiddVMWareGfxBitMapAttrBase;

static struct OOP_ABDescr attrbases[] = 
{
	{ IID_Hidd_BitMap,          &HiddBitMapAttrBase },
	{ IID_Hidd_PixFmt,          &HiddPixFmtAttrBase },
	{ IID_Hidd_Gfx,             &HiddGfxAttrBase },
	{ IID_Hidd_Sync,            &HiddSyncAttrBase },
	/* Private bases */
	{ IID_Hidd_VMWareGfx,       &HiddVMWareGfxAttrBase},
	{ IID_Hidd_VMWareGfxBitMap, &HiddVMWareGfxBitMapAttrBase},
	{ NULL, NULL }
};

#define MNAME_ROOT(x) VMWareOnBM__Root__ ## x
#define MNAME_BM(x) VMWareOnBM__Hidd_BitMap__ ## x

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define OnBitmap 1
#include "bitmap_common.c"

/*********** BitMap::New() *************************************/

OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg) {

	EnterFunc(bug("VMWareGfx.BitMap::New()\n"));
	o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
	if (o)
	{
		struct BitmapData *data;
		LONG multi=1;
		OOP_Object *pf;
		IPTR width, height, depth;
		HIDDT_ModeID modeid;
		OOP_Object *sync;
		ULONG pixelc;

		data = OOP_INST_DATA(cl, o);
		/* clear all data  */
		memset(data, 0, sizeof(struct BitmapData));
		/* Get attr values */
		OOP_GetAttr(o, aHidd_BitMap_Width,		&width);
		OOP_GetAttr(o, aHidd_BitMap_Height, 	&height);
		OOP_GetAttr(o,  aHidd_BitMap_PixFmt,	(IPTR *)&pf);
		OOP_GetAttr(pf, aHidd_PixFmt_Depth,		&depth);
		ASSERT (width != 0 && height != 0 && depth != 0);
		/* 
			We must only create depths that are supported by the friend drawable
			Currently we only support the default depth
		*/

		width=(width+15) & ~15;
		data->width = width;
		data->height = height;
		data->bpp = depth;
		data->disp = -1;
		if (depth>16)
			multi = 4;
		else if (depth>8)
			multi = 2;
		data->bytesperpix = multi;
		data->data = &XSD(cl)->data;
		data->mouse = &XSD(cl)->mouse;
		data->VideoData = data->data->vrambase;
		/* We should be able to get modeID from the bitmap */
		OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
		if (modeid != vHidd_ModeID_Invalid)
		{
			/*
				Because of not defined BitMap_Show method show 
				bitmap immediately
			*/
			setModeVMWareGfx(&XSD(cl)->data, width, height);
			XSD(cl)->visible = data;	/* Set created object as visible */
			ReturnPtr("VMWareGfx.BitMap::New()", OOP_Object *, o);
		}
		{
			OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
			OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
		}
		o = NULL;
	} /* if created object */
	ReturnPtr("VMWareGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/

VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg) {
struct BitmapData *data = OOP_INST_DATA(cl, o);

	EnterFunc(bug("VMWareGfx.BitMap::Dispose()\n")); 
	OOP_DoSuperMethod(cl, o, msg);
	ReturnVoid("VMWareGfx.BitMap::Dispose");
}

/*** init_onbmclass *********************************************************/

static int VMWareOnBM_Init(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("VMWareOnBM_Init\n"));

    ReturnInt("VMWareOnBM_Init", ULONG, OOP_ObtainAttrBases(attrbases));
}

/*** free_bitmapclass *********************************************************/

static int VMWareOnBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("VMWareOnBM_Expunge\n"));

    OOP_ReleaseAttrBases(attrbases);

    ReturnInt("VMWareOnBM_Expunge", int, TRUE);
}

ADD2INITLIB(VMWareOnBM_Init, 0)
ADD2EXPUNGELIB(VMWareOnBM_Expunge, 0)

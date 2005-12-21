/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
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
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "onbitmap.h"
#include "bitmap.h"
#include "vmwaregfxclass.h"

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

#define MNAME(x) vmwaregfxonbitmap_ ## x

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define OnBitmap 1
#include "bitmap_common.c"

/*********** BitMap::New() *************************************/

static OOP_Object *MNAME(new)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg) {

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

STATIC VOID MNAME(dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg) {
struct BitmapData *data = OOP_INST_DATA(cl, o);

	EnterFunc(bug("VMWareGfx.BitMap::Dispose()\n")); 
	OOP_DoSuperMethod(cl, o, msg);
	ReturnVoid("VMWareGfx.BitMap::Dispose");
}

/*** init_onbmclass *********************************************************/

#undef XSD
#define XSD(cl) xsd

#define NUM_ROOT_METHODS   3
#define NUM_BITMAP_METHODS 10

OOP_Class *init_vmwaregfxonbmclass(struct VMWareGfx_staticdata *xsd) {
struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
{
	{(IPTR (*)())MNAME(new)    , moRoot_New    },
	{(IPTR (*)())MNAME(dispose), moRoot_Dispose},
//	{(IPTR (*)())MNAME(set)	   , moRoot_Set},
	{(IPTR (*)())MNAME(get)	   , moRoot_Get},
	{NULL, 0UL}
};
struct OOP_MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
{
	{(IPTR (*)())MNAME(setcolors),          moHidd_BitMap_SetColors},
	{(IPTR (*)())MNAME(putpixel),           moHidd_BitMap_PutPixel},
	{(IPTR (*)())MNAME(clear),              moHidd_BitMap_Clear},
	{(IPTR (*)())MNAME(getpixel),           moHidd_BitMap_GetPixel},
/*	{(IPTR (*)())MNAME(drawpixel),          moHidd_BitMap_DrawPixel},*/
	{(IPTR (*)())MNAME(fillrect),           moHidd_BitMap_FillRect},
	{(IPTR (*)())MNAME(getimage),           moHidd_BitMap_GetImage},
	{(IPTR (*)())MNAME(putimage),           moHidd_BitMap_PutImage},
	{(IPTR (*)())MNAME(blitcolorexpansion), moHidd_BitMap_BlitColorExpansion},
	{(IPTR (*)())MNAME(putimagelut),        moHidd_BitMap_PutImageLUT},
	{(IPTR (*)())MNAME(getimagelut),        moHidd_BitMap_GetImageLUT},
	{NULL, 0UL}
};
struct OOP_InterfaceDescr ifdescr[] =
{
	{root_descr,    IID_Root       , NUM_ROOT_METHODS},
	{bitMap_descr,  IID_Hidd_BitMap, NUM_BITMAP_METHODS},
	{NULL, NULL, 0}
};
OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);
struct TagItem tags[] =
{
	{aMeta_SuperID,        (IPTR) CLID_Hidd_BitMap},
	{aMeta_InterfaceDescr, (IPTR) ifdescr},
	{aMeta_InstSize,       (IPTR) sizeof(struct BitmapData)},
	{TAG_DONE, 0UL}
};
OOP_Class *cl = NULL;

	EnterFunc(bug("init_bitmapclass(xsd=%p)\n", xsd));
	D(bug("Metattrbase: %x\n", MetaAttrBase));
	if(MetaAttrBase)
	{
		D(bug("Got attrbase\n"));
/*		for (;;) {cl = cl; } */
		cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
		if(cl)
		{
			D(bug("BitMap class ok\n"));
			xsd->onbmclass = cl;
			cl->UserData     = (APTR) xsd;
			/* Get attrbase for the BitMap interface */
			if (OOP_ObtainAttrBases(attrbases))
			{
				OOP_AddClass(cl);
			}
			else
			{
				free_vmwaregfxonbmclass( xsd );
				cl = NULL;
			}
		}
		/* We don't need this anymore */
		OOP_ReleaseAttrBase(IID_Meta);
	} /* if(MetaAttrBase) */
	ReturnPtr("init_onbmclass", OOP_Class *,  cl);
}

/*** free_bitmapclass *********************************************************/

void free_vmwaregfxonbmclass(struct VMWareGfx_staticdata *xsd) {
	EnterFunc(bug("free_onbmclass(xsd=%p)\n", xsd));
	if(xsd)
	{
		OOP_RemoveClass(xsd->onbmclass);
		if(xsd->onbmclass)
			OOP_DisposeObject((OOP_Object *) xsd->onbmclass);
		xsd->onbmclass = NULL;
		OOP_ReleaseAttrBases(attrbases);
	}
	ReturnVoid("free_onbmclass");
}

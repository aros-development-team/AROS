/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Class for Vesa.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <aros/symbolsets.h>
#include <devices/inputevent.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <hardware/custom.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <clib/alib_protos.h>
#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

#include "vesagfxclass.h"
#include "bitmap.h"
#include "hardware.h"

#include LC_LIBDEFS_FILE

static OOP_AttrBase HiddBitMapAttrBase;  
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddVesaGfxAttrBase;
static OOP_AttrBase HiddVesaGfxBitMapAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    {IID_Hidd_BitMap,          &HiddBitMapAttrBase},
    {IID_Hidd_VesaGfxBitMap,   &HiddVesaGfxBitMapAttrBase},
    {IID_Hidd_VesaGfx,         &HiddVesaGfxAttrBase},
    {IID_Hidd_PixFmt,          &HiddPixFmtAttrBase},
    {IID_Hidd_Sync,            &HiddSyncAttrBase},
    {IID_Hidd_Gfx,             &HiddGfxAttrBase},
    {NULL, NULL}
};

static UBYTE syncdescription[100];

OOP_Object *PCVesa__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem pftags[] =
    {
	{aHidd_PixFmt_RedShift,     0}, /*  0 */
	{aHidd_PixFmt_GreenShift,   0}, /*  1 */
	{aHidd_PixFmt_BlueShift,    0}, /*  2 */
	{aHidd_PixFmt_AlphaShift,   0}, /*  3 */
	{aHidd_PixFmt_RedMask,      0}, /*  4 */
	{aHidd_PixFmt_GreenMask,    0}, /*  5 */
	{aHidd_PixFmt_BlueMask,     0}, /*  6 */
	{aHidd_PixFmt_AlphaMask,    0}, /*  7 */
	{aHidd_PixFmt_ColorModel,   0}, /*  8 */
	{aHidd_PixFmt_Depth,        0}, /*  9 */
	{aHidd_PixFmt_BytesPerPixel,0}, /* 10 */
	{aHidd_PixFmt_BitsPerPixel, 0}, /* 11 */
	{aHidd_PixFmt_StdPixFmt,    0}, /* 12 */
	{aHidd_PixFmt_CLUTShift,    0}, /* 13 */
	{aHidd_PixFmt_CLUTMask,     0}, /* 14 */
	{aHidd_PixFmt_BitMapType,   0}, /* 15 */
	{TAG_DONE, 0UL }
    };
    struct TagItem sync_mode[] =
    {
	{aHidd_Sync_PixelClock, 0},
	{aHidd_Sync_HDisp,      0},
	{aHidd_Sync_VDisp,      0},
	{aHidd_Sync_Description,0},
	{aHidd_Sync_HSyncStart, 0},
	{aHidd_Sync_HSyncEnd,   0},
	{aHidd_Sync_HTotal,     0},
	{aHidd_Sync_VSyncStart, 0},
	{aHidd_Sync_VSyncEnd,   0},
	{aHidd_Sync_VTotal,     0},
	{TAG_DONE, 0UL}
    };
    struct TagItem modetags[] =
    {
	{aHidd_Gfx_PixFmtTags, (IPTR)pftags},
	{aHidd_Gfx_SyncTags,   (IPTR)sync_mode},
	{TAG_DONE, 0UL}
    };
    struct TagItem yourtags[] =
    {
	{aHidd_Gfx_ModeTags, (IPTR)modetags},
	{TAG_MORE, 0UL}
    };
    struct pRoot_New yourmsg;

    pftags[0].ti_Data = XSD(cl)->data.redshift;
    pftags[1].ti_Data = XSD(cl)->data.greenshift;
    pftags[2].ti_Data = XSD(cl)->data.blueshift;
    pftags[3].ti_Data = 0;
    pftags[4].ti_Data = XSD(cl)->data.redmask;
    pftags[5].ti_Data = XSD(cl)->data.greenmask;
    pftags[6].ti_Data = XSD(cl)->data.bluemask;
    pftags[7].ti_Data = 0;
    pftags[8].ti_Data = vHidd_ColorModel_TrueColor;
    pftags[9].ti_Data = (XSD(cl)->data.depth > 24) ? 24 : XSD(cl)->data.depth;
    pftags[10].ti_Data = XSD(cl)->data.bytesperpixel;
    pftags[11].ti_Data = (XSD(cl)->data.bitsperpixel > 24) ? 24 : XSD(cl)->data.bitsperpixel;
    pftags[12].ti_Data = vHidd_StdPixFmt_Native;
    pftags[15].ti_Data = vHidd_BitMapType_Chunky;

    sync_mode[1].ti_Data = XSD(cl)->data.width;
    sync_mode[2].ti_Data = XSD(cl)->data.height;
    __sprintf(syncdescription, "VESA:%ldx%ld", XSD(cl)->data.width, XSD(cl)->data.height);
    sync_mode[3].ti_Data = (IPTR)syncdescription;

    yourtags[1].ti_Data = (IPTR)msg->attrList;
    yourmsg.mID = msg->mID;
    yourmsg.attrList = yourtags;
    msg = &yourmsg;
    EnterFunc(bug("VesaGfx::New()\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	D(bug("Got object from super\n"));
	XSD(cl)->vesagfxhidd = o;
	XSD(cl)->mouse.shape = NULL;
	ReturnPtr("VesaGfx::New", OOP_Object *, o);
    }
    ReturnPtr("VesaGfx::New", OOP_Object *, NULL);
}

VOID PCVesa__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    if (XSD(cl)->mouse.shape != NULL)
	FreeVec(XSD(cl)->mouse.shape);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID PCVesa__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    BOOL found = FALSE;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    /* Our only gettable attribute is wether we support HW cursor or not.
	     * We do not have any such support, since we are a simple VESA fb driver */
	    case aoHidd_Gfx_SupportsHWCursor:
		*msg->storage = (IPTR)FALSE;
		found = TRUE;
		break;
	}
    }
    if (!found)
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object *PCVesa__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    BOOL displayable;
    BOOL framebuffer;
    OOP_Class *classptr = NULL;
    struct TagItem tags[2];
    struct pHidd_Gfx_NewBitMap yourmsg;

    EnterFunc(bug("VesaGfx::NewBitMap()\n"));
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
    if (framebuffer)
	classptr = XSD(cl)->onbmclass;
    else if (displayable)
	classptr = XSD(cl)->offbmclass;
    else
    {
	HIDDT_ModeID modeid;
	modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
	if (modeid != vHidd_ModeID_Invalid)
	    classptr = XSD(cl)->offbmclass;
	else
	{
	    HIDDT_StdPixFmt stdpf;
	    stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
	    if (stdpf == vHidd_StdPixFmt_Unknown)
	    {
		OOP_Object *friend;
		friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, NULL, msg->attrList);
		if (friend != NULL)
		{
		    OOP_Object *gfxhidd;
		    OOP_GetAttr(friend, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
		    if (gfxhidd == o)
		    {
			classptr = XSD(cl)->offbmclass;
		    }
		}
	    }
	}
    }
    if (classptr != NULL)
    {
	tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
	tags[0].ti_Data = (IPTR)classptr;
	tags[1].ti_Tag = TAG_MORE;
	tags[1].ti_Data = (IPTR)msg->attrList;
	yourmsg.mID = msg->mID;
	yourmsg.attrList = tags;
	msg = &yourmsg;
    }
    ReturnPtr("VesaGfx::NewBitMap", OOP_Object *, (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg));
}

VOID PCVesa__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    unsigned char *src = 0, *dest = 0;
    ULONG mode;

    mode = GC_DRMD(msg->gc);

    OOP_GetAttr(msg->src,  aHidd_VesaGfxBitMap_Drawable, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_VesaGfxBitMap_Drawable, (IPTR *)&dest);

    if (!dest || !src ||
    	((mode != vHidd_GC_DrawMode_Copy)))
    {
	/* The source and/or destination object is no VesaGfx bitmap, onscreen nor offscreen.
	   Or drawmode is not one of those we accelerate. Let the superclass do the
	   copying in a more general way
	*/
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	return;
	
    }

    {
    	struct BitmapData *data = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        struct BitmapData *ddata = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);

        switch(mode)
	{
	    case vHidd_GC_DrawMode_Copy:
	    	switch(data->bytesperpix)
		{
		    case 1:
	    		HIDD_BM_CopyMemBox8(msg->dest,
		    	    		    data->VideoData,
					    msg->srcX,
					    msg->srcY,
					    ddata->VideoData,
					    msg->destX,
					    msg->destY,
					    msg->width,
					    msg->height,
					    data->bytesperline,
					    ddata->bytesperline);
			break;

		    case 2:
	    		HIDD_BM_CopyMemBox16(msg->dest,
		    	    		    data->VideoData,
					    msg->srcX,
					    msg->srcY,
					    ddata->VideoData,
					    msg->destX,
					    msg->destY,
					    msg->width,
					    msg->height,
					    data->bytesperline,
					    ddata->bytesperline);
			break;
			

		    case 3:
	    		HIDD_BM_CopyMemBox24(msg->dest,
		    	    		    data->VideoData,
					    msg->srcX,
					    msg->srcY,
					    ddata->VideoData,
					    msg->destX,
					    msg->destY,
					    msg->width,
					    msg->height,
					    data->bytesperline,
					    ddata->bytesperline);
			break;

		    case 4:
	    		HIDD_BM_CopyMemBox32(msg->dest,
		    	    		    data->VideoData,
					    msg->srcX,
					    msg->srcY,
					    ddata->VideoData,
					    msg->destX,
					    msg->destY,
					    msg->width,
					    msg->height,
					    data->bytesperline,
					    ddata->bytesperline);
			break;
		    	
	    	} /* switch(data->bytesperpix) */
    	    	break;
		
    	} /* switch(mode) */
	
    #if BUFFERED_VRAM
	if (OOP_OCLASS(msg->dest) == XSD(cl)->onbmclass)
	{
    	    LOCK_FRAMEBUFFER(XSD(cl));    
    	    vesaRefreshArea(ddata, msg->destX, msg->destY, msg->destX + msg->width - 1, msg->destY + msg->height - 1);
    	    UNLOCK_FRAMEBUFFER(XSD(cl));
	}
    #endif
    
    } /**/

}

VOID PCVesa__Hidd_Gfx__ShowImminentReset(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    memset(XSD(cl)->data.framebuffer,
    	   0,
	   XSD(cl)->data.height * XSD(cl)->data.bytesperline);
}


AROS_SET_LIBFUNC(PCVesa_InitClass, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    EnterFunc(bug("PCVesa_InitClass\n"));

    LIBBASE->vsd.mouse.x=0;
    LIBBASE->vsd.mouse.y=0;
    LIBBASE->vsd.mouse.shape = NULL;

    ReturnInt("PCVesa_InitClass", ULONG, OOP_ObtainAttrBases(attrbases));
    
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(PCVesa_ExpungeClass, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    EnterFunc(bug("free_vesagfxclass(xsd=%p)\n", xsd));

    OOP_ReleaseAttrBases(attrbases);
    ReturnInt("PCVesa_ExpungeClass", ULONG, TRUE);
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(PCVesa_InitClass, 0)
ADD2EXPUNGELIB(PCVesa_ExpungeClass, 0)

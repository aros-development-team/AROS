/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Offscreen bitmap class for Display hidd.
    Lang: English.
*/


#include <proto/oop.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <exec/lists.h>

#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>
#include <exec/alerts.h>

#include <hidd/graphics.h>

#include <string.h>

#include "display.h"
#include "displayclass.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "bitmap.h"

#include "assert.h"
/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

#if 0
static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddDisplayGfxAB;
static OOP_AttrBase HiddDisplayBitMapAB;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase },
    { IID_Hidd_PixFmt,		&HiddPixFmtAttrBase },
    { IID_Hidd_Gfx,		&HiddGfxAttrBase },
    /* Private bases */
    { IID_Hidd_Displaygfx,	&HiddDisplayGfxAB	},
    { IID_Hidd_DisplayBitMap,	&HiddDisplayBitMapAB },
    { NULL, NULL }
};
#endif

void free_offbmclass(struct display_staticdata *);
void DisplayRefreshArea(struct bitmap_data *, int , struct Box *);

#define MNAME(x) offbitmap_ ## x

#include "bitmap_common.c"

/*********** BitMap::New() *************************************/

static OOP_Object *offbitmap_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("DisplayGfx.BitMap::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
    	struct bitmap_data *data;
        IPTR width, height, depth;
	
	OOP_Object *friend, *pf;
	
        data = OOP_INST_DATA(cl, o);
	
	/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
	/* Get attr values */
#define xsd XSD(cl)
	OOP_GetAttr(o, aHidd_BitMap_Width,		&width);
	OOP_GetAttr(o, aHidd_BitMap_Height, 	&height);
#if 0
/* nlorentz: The aHidd_BitMap_Depth attribute no loner exist,, so we must
	get the depth in two steps: First get pixel format, then get depth */
	OOP_GetAttr(o, aHidd_BitMap_Depth,		&depth);
#else
	OOP_GetAttr(o,  aHidd_BitMap_PixFmt,	(IPTR *)&pf);
	OOP_GetAttr(pf, aHidd_PixFmt_Depth,		&depth);
#endif
	
	/* Get the friend bitmap. This should be a displayable bitmap */
	OOP_GetAttr(o, aHidd_BitMap_Friend,	(IPTR *)&friend);
#undef xsd
	/* If you got a friend bitmap, copy its colormap */
	if (friend)
	{
	    struct bitmap_data *src = OOP_INST_DATA(cl, friend);
	    
	    CopyMem(&src->cmap, &data->cmap, 4*16);
	}
	

//	assert (width != 0 && height != 0 && depth != 0);
	
	/* 
	   We must only create depths that are supported by the friend drawable
	   Currently we only support the default depth
	*/
	
/* nlorentz: With the new HIDD design we decided in Gfx::NewBitMap()
    that we should only create bitmaps that are alike to the friend bitmap.
    Thus the test below is really not necessary, as we will allways
    get the same depth
*/
	if (depth != 4)
	{
//	    depth = 4;	/* Do anything... */
	}

#if 0
    /* nlorentz: Not necessary with the new design */	
	/* Update the depth to the one we use */
	depth_tags[0].ti_Data = depth;
	SetAttrs(o, depth_tags);
#endif
	width=(width+15) & ~15;

	data->width = width;
	data->height = height;
	data->bpp = depth;
	data->disp = 0;
	data->VideoData = AllocVec(width*height,MEMF_PUBLIC|MEMF_CLEAR);
	if (data->VideoData)
	{
	    data->Regs = AllocVec(sizeof(struct DisplayHWRec),MEMF_PUBLIC|MEMF_CLEAR);
	    if (data->Regs)
	    {
#if 0
    /* nlorentz: Not necessary nor possible with the new design */
		set_pixelformat(o);
#endif
		if (XSD(cl)->activecallback)
		    XSD(cl)->activecallback(XSD(cl)->callbackdata, o, TRUE);

		ReturnPtr("DisplayGfx.BitMap::New()", Object *, o);
	    }
	} /* if got data->VideoData */

	{
	    OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
    	    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	}
	
	o = NULL;
    } /* if created object */

    ReturnPtr("DisplayGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/

static VOID offbitmap_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    EnterFunc(bug("DisplayGfx.BitMap::Dispose()\n"));
    
    if (data->VideoData)
	FreeVec(data->VideoData);
    if (data->Regs)
	FreeVec(data->Regs);
	
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("DisplayGfx.BitMap::Dispose");
}


#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>



/*** init_bmclass *********************************************************/

#undef XSD
#define XSD(cl) xsd


#define NUM_ROOT_METHODS   3
#define NUM_BITMAP_METHODS 10


OOP_Class *init_offbmclass(struct display_staticdata *xsd)
{
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())MNAME(new), 	moRoot_New    },
        {(IPTR (*)())MNAME(dispose),	moRoot_Dispose},
#if 0
        {(IPTR (*)())MNAME(set),	moRoot_Set},
#endif
        {(IPTR (*)())MNAME(get),	moRoot_Get},
        {NULL, 0UL}
    };

    struct OOP_MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())MNAME(setcolors),		moHidd_BitMap_SetColors},
    	{(IPTR (*)())MNAME(putpixel),		moHidd_BitMap_PutPixel},
    	{(IPTR (*)())MNAME(clear),		moHidd_BitMap_Clear},
    	{(IPTR (*)())MNAME(getpixel),		moHidd_BitMap_GetPixel},
/*    	{(IPTR (*)())MNAME(drawpixel),		moHidd_BitMap_DrawPixel},*/
    	{(IPTR (*)())MNAME(fillrect),		moHidd_BitMap_FillRect},
    	{(IPTR (*)())MNAME(getimage),		moHidd_BitMap_GetImage},
    	{(IPTR (*)())MNAME(putimage),		moHidd_BitMap_PutImage},
    	{(IPTR (*)())MNAME(blitcolorexpansion),	moHidd_BitMap_BlitColorExpansion},
    	{(IPTR (*)())MNAME(putimagelut),	moHidd_BitMap_PutImageLUT},
    	{(IPTR (*)())MNAME(getimagelut),	moHidd_BitMap_GetImageLUT},
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
        {aMeta_InstSize,       (IPTR) sizeof(struct bitmap_data)},
        {TAG_DONE, 0UL}
    };
    
    OOP_Class *cl = NULL;

    EnterFunc(bug("init_bitmapclass(xsd=%p)\n", xsd));
    
    
    D(bug("Metattrbase: %x\n", MetaAttrBase));


    if(MetaAttrBase)
    {
       D(bug("Got attrbase\n"));
       
/*    for (;;) {cl = cl; } */
        cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("BitMap class ok\n"));
            xsd->offbmclass = cl;
            cl->UserData     = (APTR) xsd;

            __IHidd_BitMap = OOP_ObtainAttrBase(IID_Hidd_BitMap);
            __IHidd_PixFmt = OOP_ObtainAttrBase(IID_Hidd_PixFmt);
            __IHidd_Gfx    = OOP_ObtainAttrBase(IID_Hidd_Gfx);
            __IHidd_DisplayGfx    = OOP_ObtainAttrBase(IID_Hidd_Displaygfx);
            __IHidd_DisplayBitMap = OOP_ObtainAttrBase(IID_Hidd_DisplayBitMap);
           
            /* Get attrbase for the BitMap interface */
	    if (NULL != __IHidd_BitMap &&
	        NULL != __IHidd_PixFmt &&
	        NULL != __IHidd_Gfx    &&
	        NULL != __IHidd_DisplayGfx &&
	        NULL != __IHidd_DisplayBitMap)
            {
                OOP_AddClass(cl);
            }
            else
            {
                free_offbmclass( xsd );
                cl = NULL;
            }
        }
	
	/* We don't need this anymore */
	OOP_ReleaseAttrBase(IID_Meta);
    } /* if(MetaAttrBase) */

    ReturnPtr("init_bmclass", Class *,  cl);
}


/*** free_offbitmapclass *********************************************************/

void free_offbmclass(struct display_staticdata *xsd)
{
    EnterFunc(bug("free_bmclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        OOP_RemoveClass(xsd->offbmclass);
        if(xsd->offbmclass) OOP_DisposeObject((OOP_Object *) xsd->offbmclass);
        xsd->offbmclass = NULL;

#warning Change this!
        if (NULL != __IHidd_BitMap)
        	OOP_ReleaseAttrBase(IID_Hidd_BitMap);
        if (NULL != __IHidd_PixFmt)
        	OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
        if (NULL != __IHidd_Gfx)
        	OOP_ReleaseAttrBase(IID_Hidd_Gfx);
        if (NULL != __IHidd_DisplayGfx)
        	OOP_ReleaseAttrBase(IID_Hidd_Displaygfx);
        if (NULL != __IHidd_DisplayBitMap)
        	OOP_ReleaseAttrBase(IID_Hidd_DisplayBitMap);
    }
    ReturnVoid("free_bmclass");
}

/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Offscreen bitmap class for VGA hidd.
    Lang: English.
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/oop.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <exec/lists.h>

#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>
#include <exec/alerts.h>

#include <hidd/graphics.h>

#include "vga.h"
#include "vgaclass.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "bitmap.h"

static AttrBase HiddBitMapAttrBase = 0;
static AttrBase HiddVGAGfxAB = 0;
static AttrBase HiddVGABitMapAB = 0;

static struct abdescr attrbases[] = 
{
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase },
    /* Private bases */
    { IID_Hidd_VGAgfx,		&HiddVGAGfxAB	},
    { IID_Hidd_VGABitMap,	&HiddVGABitMapAB },
    { NULL, NULL }
};

void free_offbmclass(struct vga_staticdata *);
void vgaRefreshArea(struct bitmap_data *, int , struct Box *);

#define MNAME(x) offbitmap_ ## x

#include "bitmap_common.c"

/*********** BitMap::New() *************************************/

static Object *offbitmap_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("VGAGfx.BitMap::New()\n"));
    
    o = (Object *)DoSuperMethod(cl, o, (Msg) msg);
    if (o)
    {
    	struct bitmap_data *data;
	struct TagItem depth_tags[] = {
	    { aHidd_BitMap_Depth, 0 },
	    { TAG_DONE, 0 }
	};
	
        IPTR width, height, depth;
	
	Object *friend;
	
        data = INST_DATA(cl, o);
	
	/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
	/* Get attr values */
	GetAttr(o, aHidd_BitMap_Width,		&width);
	GetAttr(o, aHidd_BitMap_Height, 	&height);
	GetAttr(o, aHidd_BitMap_Depth,		&depth);
	
	/* Get the friend bitmap. This should be a displayable bitmap */
	GetAttr(o, aHidd_BitMap_Friend,	(IPTR *)&friend);

	/* If you got a friend bitmap, copy its colormap */
	if (friend)
	{
	    struct bitmap_data *src = INST_DATA(cl, friend);
	    
	    CopyMem(&src->cmap, &data->cmap, 4*16);
	}
	
	assert (width != 0 && height != 0 && depth != 0);
	
	/* 
	   We must only create depths that are supported by the friend drawable
	   Currently we only support the default depth
	*/
	
	if (depth != 4)
	{
//	    depth = 4;	/* Do anything... */
	}
	
	/* Update the depth to the one we use */
	depth_tags[0].ti_Data = depth;
	SetAttrs(o, depth_tags);
	
	data->width = width;
	data->height = height;
	data->bpp = depth;
	data->disp = 0;
	width=(width+15) & ~15;
	data->VideoData = AllocVec(width*height,MEMF_PUBLIC|MEMF_CLEAR);
	if (data->VideoData)
	{
	    data->Regs = AllocVec(sizeof(struct vgaHWRec),MEMF_PUBLIC|MEMF_CLEAR);
	    if (data->Regs)
	    {
		set_pixelformat(o, XSD(cl));
		ReturnPtr("VGAGfx.BitMap::New()", Object *, o);
	    }
	} /* if got data->VideoData */

	{
	    MethodID disp_mid = GetMethodID(IID_Root, moRoot_Dispose);
    	    CoerceMethod(cl, o, (Msg) &disp_mid);
	}
	
	o = NULL;
    } /* if created object */

    ReturnPtr("VGAGfx.BitMap::New()", Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/

static VOID offbitmap_dispose(Class *cl, Object *o, Msg msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    EnterFunc(bug("VGAGfx.BitMap::Dispose()\n"));
    
    if (data->VideoData)
	FreeVec(data->VideoData);
	
    DoSuperMethod(cl, o, msg);
    
    ReturnVoid("VGAGfx.BitMap::Dispose");
}

static BOOL offbitmap_setcolors(Class *cl, Object *o, struct pHidd_BitMap_SetColors *msg)
{
    return bitmap_setcolors(cl, o, msg);
}


BOOL bitmap_setcolors(Class *cl, Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    
    ULONG xc_i,col_i;
    
    EnterFunc(bug("VGAGfx.BitMap::SetColors(num=%d, first=%d)\n",
    		msg->numColors, msg->firstColor));
    
    if (data->Regs)
    {
        for ( xc_i = msg->firstColor, col_i = 0;
    		col_i < msg ->numColors; 
		xc_i ++, col_i ++ )
	{
	    data->Regs->DAC[xc_i*3+0] = msg->colors[col_i].red >> 10;
	    data->Regs->DAC[xc_i*3+1] = msg->colors[col_i].green >> 10;
	    data->Regs->DAC[xc_i*3+2] = msg->colors[col_i].blue >> 10;
	}
    }

    ReturnBool("VGAGfx.BitMap::SetColors",  TRUE);
}


#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 1
#include <aros/debug.h>



/*** init_bmclass *********************************************************/

#undef XSD
#define XSD(cl) xsd

//#define NUM_BITMAP_METHODS 14
#define NUM_ROOT_METHODS   4
#define NUM_BITMAP_METHODS 9


Class *init_offbmclass(struct vga_staticdata *xsd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())MNAME(new), 	moRoot_New    },
        {(IPTR (*)())MNAME(dispose),	moRoot_Dispose},
        {(IPTR (*)())MNAME(set),	moRoot_Set},
        {(IPTR (*)())MNAME(get),	moRoot_Get},
        {NULL, 0UL}
    };

    struct MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
      {(IPTR (*)())MNAME(setcolors),		moHidd_BitMap_SetColors},
    	{(IPTR (*)())MNAME(putpixel),		moHidd_BitMap_PutPixel},
    	{(IPTR (*)())MNAME(clear),		moHidd_BitMap_Clear},
    	{(IPTR (*)())MNAME(getpixel),		moHidd_BitMap_GetPixel},
    	{(IPTR (*)())MNAME(drawpixel),		moHidd_BitMap_DrawPixel},
//    	{(IPTR (*)())MNAME(fillrect),		moHidd_BitMap_FillRect},
    	{(IPTR (*)())MNAME(copybox),		moHidd_BitMap_CopyBox},
//    	{(IPTR (*)())MNAME(getimage),		moHidd_BitMap_GetImage},
    	{(IPTR (*)())MNAME(putimage),		moHidd_BitMap_PutImage},
//    	{(IPTR (*)())MNAME(blitcolorexpansion),	moHidd_BitMap_BlitColorExpansion},
    	{(IPTR (*)())MNAME(mapcolor),		moHidd_BitMap_MapColor},
    	{(IPTR (*)())MNAME(unmappixel),		moHidd_BitMap_UnmapPixel},
//    	{(IPTR (*)())MNAME(putimagelut),	moHidd_BitMap_PutImageLUT},
//    	{(IPTR (*)())MNAME(getimagelut),	moHidd_BitMap_GetImageLUT},
        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , NUM_ROOT_METHODS},
        {bitMap_descr,  IID_Hidd_BitMap, NUM_BITMAP_METHODS},
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Hidd_BitMap},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_InstSize,       (IPTR) sizeof(struct bitmap_data)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_bitmapclass(xsd=%p)\n", xsd));
    
    
    D(bug("Metattrbase: %x\n", MetaAttrBase));


    if(MetaAttrBase)
    {
       D(bug("Got attrbase\n"));
       
/*    for (;;) {cl = cl; } */
        cl = NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("BitMap class ok\n"));
            xsd->offbmclass = cl;
            cl->UserData     = (APTR) xsd;
           
            /* Get attrbase for the BitMap interface */
	    if (obtainattrbases(attrbases, OOPBase))
            {
                AddClass(cl);
            }
            else
            {
                free_offbmclass( xsd );
                cl = NULL;
            }
        }
	
	/* We don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    } /* if(MetaAttrBase) */

    ReturnPtr("init_bmclass", Class *,  cl);
}


/*** free_offbitmapclass *********************************************************/

void free_offbmclass(struct vga_staticdata *xsd)
{
    EnterFunc(bug("free_bmclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        RemoveClass(xsd->offbmclass);
        if(xsd->offbmclass) DisposeObject((Object *) xsd->offbmclass);
        xsd->offbmclass = NULL;
	
	releaseattrbases(attrbases, OOPBase);
    }
    ReturnVoid("free_bmclass");
}

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: onbitmap.c 30792 2009-03-07 22:40:04Z neil $

    Desc: Bitmap class for VGA hidd.
    Lang: English.
*/

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <exec/lists.h>

#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>

#include <aros/symbolsets.h>

#include <hidd/graphics.h>

#include <assert.h>

#include "vgahw.h"
#include "vga.h"
#include "vgaclass.h"

#include "bitmap.h"

#include LC_LIBDEFS_FILE

/* Don't initialize static variables with "=0", otherwise they go into DATA segment */

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddVGAGfxAB;
static OOP_AttrBase HiddVGABitMapAB;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase },
    { IID_Hidd_PixFmt,		&HiddPixFmtAttrBase },
    { IID_Hidd_Gfx,		&HiddGfxAttrBase },
    { IID_Hidd_Sync,		&HiddSyncAttrBase },
    /* Private bases */
    { IID_Hidd_VGAgfx,		&HiddVGAGfxAB	},
    { IID_Hidd_VGABitMap,	&HiddVGABitMapAB },
    { NULL, NULL }
};

void vgaRestore(struct vgaHWRec *, BOOL onlyDAC);
void * vgaSave(struct vgaHWRec *);
int vgaInitMode(struct vgaModeDesc *, struct vgaHWRec *);
void vgaLoadPalette(struct vgaHWRec *, unsigned char *);

#define MNAME_ROOT(x) PCVGABM__Root__ ## x
#define MNAME_BM(x) PCVGABM__Hidd_BitMap__ ## x

/*********** BitMap::New() *************************************/

OOP_Object *PCVGABM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("VGAGfx.BitMap::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    D(bug("[VGABitMap] Object created by superclass: 0x%p\n"));
    if (o)
    {
    	struct bitmap_data *data;
	OOP_Object *pf;
        IPTR width, height, depth;
	IPTR displayable = FALSE;

        data = OOP_INST_DATA(cl, o);

	/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
	/* Get attr values */
	OOP_GetAttr(o, aHidd_BitMap_Width, &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);
	OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);
	OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
	D(bug("[VGABitMap] PixFmt object: 0x%p\n", pf));
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

	D(bug("[VGABitMap] Size: %lux%lu, depth: %lu\n", width, height, depth));
	D(bug("[VGABitMap] Displayable: %ld\n", displayable));
	ASSERT (width != 0 && height != 0 && depth != 0);
	
	/* 
	   We must only create depths that are supported by the friend drawable
	   Currently we only support the default depth
	 */

	data->width = width;
	data->height = height;
	data->bpp = depth;
	width=(width+15) & ~15;
	data->VideoData = AllocVec(width*height,MEMF_PUBLIC|MEMF_CLEAR);
	D(bug("[VGABitMap] Allocated videodata at 0x%p\n", data->VideoData));
	if (data->VideoData) {
	    /* If the bitmap is not displayable, we're done */
	    if (!displayable)
		ReturnPtr("VGAGfx.BitMap::New()", OOP_Object *, o);

	    data->Regs = AllocVec(sizeof(struct vgaHWRec),MEMF_PUBLIC|MEMF_CLEAR);
	    D(bug("[VGABitMap] Registers at 0x%p\n", data->Regs));
	    /*
	      Here there is brand new method of getting pixelclock data.
	      It was introduced here to make the code more portable. Besides
	      it may now be used as a base for creating other low level
	      video drivers
	    */
	    if (data->Regs) {
		struct vgaModeDesc mode;
		HIDDT_ModeID modeid;
		OOP_Object *sync;
		OOP_Object *pf;
		IPTR pixelc;
		
		/* We should be able to get modeID from the bitmap */
		OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
				
		if (modeid != vHidd_ModeID_Invalid)
		{
		    /* Get Sync and PixelFormat properties */
		    HIDD_Gfx_GetMode(XSD(cl)->vgahidd, modeid, &sync, &pf);

		    mode.Width 	= width;
		    mode.Height = height;
		    mode.Depth 	= depth;
		    OOP_GetAttr(sync, aHidd_Sync_PixelClock, &pixelc);

		    mode.clock	= (pixelc > 26000000) ? 1 : 0;
		    mode.Flags	= 0;
		    mode.HSkew	= 0;
		    OOP_GetAttr(sync, aHidd_Sync_HDisp, 	&mode.HDisplay);
		    OOP_GetAttr(sync, aHidd_Sync_VDisp, 	&mode.VDisplay);
		    OOP_GetAttr(sync, aHidd_Sync_HSyncStart, 	&mode.HSyncStart);
		    OOP_GetAttr(sync, aHidd_Sync_VSyncStart, 	&mode.VSyncStart);
		    OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,	&mode.HSyncEnd);
		    OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,	&mode.VSyncEnd);
		    OOP_GetAttr(sync, aHidd_Sync_HTotal,	&mode.HTotal);
		    OOP_GetAttr(sync, aHidd_Sync_VTotal,	&mode.VTotal);

		    /* Now, when the best display mode is chosen, we can build it */
		    vgaInitMode(&mode, data->Regs);
		    vgaLoadPalette(data->Regs,(unsigned char *)NULL);

		    D(bug("[VGABitMap] Created displayable bitmap 0x%p, data 0x%p\n", o, data)); 
		    return o;
		}
	    }
	    FreeVec(data->VideoData);
	}

	{
	    OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	}

	o = NULL;
    } /* if created object */

    ReturnPtr("VGAGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/

VOID PCVGABM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    EnterFunc(bug("VGAGfx.BitMap::Dispose()\n"));
    
    if (data->VideoData)
	FreeVec(data->VideoData);
    if (data->Regs)
	FreeVec(data->Regs);
    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("VGAGfx.BitMap::Dispose");
}

/*** init_bmclass *********************************************************/

static int PCVGABM_Init(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("PCVGABM_Init\n"));
    
    ReturnInt("PCVGABM_Init", ULONG, OOP_ObtainAttrBases(attrbases));
}

/*** expunge_bmclass *******************************************************/

static int PCVGABM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("PCCVGABM_Expunge\n"));

    OOP_ReleaseAttrBases(attrbases);
    ReturnInt("PCVGABM_Expunge", int, TRUE);
}

/*****************************************************************************/

ADD2INITLIB(PCVGABM_Init, 0)
ADD2EXPUNGELIB(PCVGABM_Expunge, 0)

/*********  BitMap::Clear()  *************************************/
VOID MNAME_BM(Clear)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    IPTR width, height;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    struct Box box = {0, 0, 0, 0};
    
    /* Get width & height from bitmap superclass */

    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);

    box.x2 = width - 1;
    box.y2 = height - 1;

    memset(data->VideoData, GC_BG(msg->gc), width*height);

    if (data->disp) {
	ObtainSemaphore(&XSD(cl)->HW_acc);
	vgaRefreshArea(data, 1, &box);
	draw_mouse(XSD(cl));
	ReleaseSemaphore(&XSD(cl)->HW_acc);
    }

    return;
}

void vgaDACLoad(struct vgaHWRec *, unsigned char, int);

BOOL MNAME_BM(SetColors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat *pf;
    
    ULONG xc_i, col_i;
    
    HIDDT_Pixel	red, green, blue;
    
    pf = BM_PIXFMT(o);

    if (    vHidd_ColorModel_StaticPalette == HIDD_PF_COLMODEL(pf)
    	 || vHidd_ColorModel_TrueColor	   == HIDD_PF_COLMODEL(pf) ) {
	 
	 /* Superclass takes care of this case */
	 
	 return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    /* We have a vHidd_GT_Palette bitmap */    
    
    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) return FALSE;
    if (!data->Regs)
	return TRUE;
    
    if ((msg->firstColor + msg->numColors) > (1 << data->bpp))
	return FALSE;
    
    
    for ( xc_i = msg->firstColor, col_i = 0;
    		col_i < msg->numColors; 
		xc_i ++, col_i ++ )
    {
	red   = msg->colors[col_i].red   >> 8;
	green = msg->colors[col_i].green >> 8;
	blue  = msg->colors[col_i].blue  >> 8;

	/* Update DAC registers */
	data->Regs->DAC[xc_i*3] = red >> 2;
	data->Regs->DAC[xc_i*3+1] = green >> 2;
	data->Regs->DAC[xc_i*3+2] = blue >> 2;
	
	msg->colors[col_i].pixval = xc_i;
    }

    /* Restore palette if displayed */
    if (data->disp) {
	ObtainSemaphore(&XSD(cl)->HW_acc);
	vgaDACLoad(data->Regs, msg->firstColor, msg->numColors);
	ReleaseSemaphore(&XSD(cl)->HW_acc);
    }

    return TRUE;
}

/*********  BitMap::PutPixel()  ***************************/

VOID MNAME_BM(PutPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel fg;
    unsigned char *ptr;

    fg = msg->pixel;
    ptr = (char *)(data->VideoData + msg->x + (msg->y * data->width));
    *ptr = (char) fg;

    if (data->disp) {
	int pix;
	unsigned char *ptr2;

	ptr2 = (char *)(0xa0000 + (msg->x + (msg->y * data->width)) / 8);
	pix = 0x8000 >> (msg->x % 8);
	ObtainSemaphore(&XSD(cl)->HW_acc);

	outw(0x3c4,0x0f02);
	outw(0x3ce,pix | 8);
	outw(0x3ce,0x0005);
	outw(0x3ce,0x0003);
	outw(0x3ce,(fg << 8));
	outw(0x3ce,0x0f01);

	*ptr2 |= 1;		// This or'ed value isn't important

	if (((msg->x >= XSD(cl)->mouseX) && (msg->x < (XSD(cl)->mouseX + XSD(cl)->mouseW))) ||
	    ((msg->y >= XSD(cl)->mouseY) && (msg->y < (XSD(cl)->mouseY + XSD(cl)->mouseH))))
	    draw_mouse(XSD(cl));

        ReleaseSemaphore(&XSD(cl)->HW_acc);
    }
    return;
}

/*********  BitMap::GetPixel()  *********************************/
HIDDT_Pixel MNAME_BM(GetPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel=0;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    
    unsigned char *ptr;

    ptr = (char *)(data->VideoData + msg->x + (msg->y * data->width));

    pixel = *(char*)ptr;

    /* Get pen number from colortab */
    return pixel;
}

/*********  BitMap::PutImage()  ***************************/

VOID MNAME_BM(PutImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    struct Box      	box = {0, 0, 0, 0};
    BOOL    	    	done_by_superclass = FALSE;
    
    EnterFunc(bug("VGAGfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	msg->pixels, msg->x, msg->y, msg->width, msg->height));

    switch(msg->pixFmt)
    {
    	case vHidd_StdPixFmt_Native:
	    HIDD_BM_CopyMemBox8(o,
		    	    	msg->pixels,
				0,
				0,
				data->VideoData,
				msg->x,
				msg->y,
				msg->width,
				msg->height,
				msg->modulo,
				data->width);
	    break;
	    
   	case vHidd_StdPixFmt_Native32:
	    HIDD_BM_PutMem32Image8(o,
		    	    	   msg->pixels,
				   data->VideoData,
				   msg->x,
				   msg->y,
				   msg->width,
				   msg->height,
				   msg->modulo,
				   data->width);
	    break;
	    
	default:
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    done_by_superclass = TRUE;
	    break;
	    
    }
	    
    if (data->disp && !done_by_superclass)
    {
        box.x1 = msg->x;
        box.y1 = msg->y;
        box.x2 = box.x1 + msg->width - 1;
        box.y2 = box.y1 + msg->height - 1;

        ObtainSemaphore(&XSD(cl)->HW_acc);
	
        vgaRefreshArea(data, 1, &box);
	
	if ( (	(XSD(cl)->mouseX + XSD(cl)->mouseW - 1 >= box.x1) &&
		(XSD(cl)->mouseX <= box.x2) ) ||
	    (	(XSD(cl)->mouseY + XSD(cl)->mouseH - 1 >= box.y1) && 
		(XSD(cl)->mouseY <= box.y2) ) )
	    draw_mouse(XSD(cl));
	    
        ReleaseSemaphore(&XSD(cl)->HW_acc);
    }
    
    ReturnVoid("VGAGfx.BitMap::PutImage");
}

/*********  BitMap::GetImage()  ***************************/

VOID MNAME_BM(GetImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct bitmap_data  *data = OOP_INST_DATA(cl, o);

    switch(msg->pixFmt)
    {
    	case vHidd_StdPixFmt_Native:
	    HIDD_BM_CopyMemBox8(o,
		    	    	data->VideoData,
				msg->x,
				msg->y,
				msg->pixels,
				0,
				0,
				msg->width,
				msg->height,
				data->width,
				msg->modulo);
	    break;
	    
    	case vHidd_StdPixFmt_Native32:
	    HIDD_BM_GetMem32Image8(o,
		    	    	   data->VideoData,
				   msg->x,
				   msg->y,
				   msg->pixels,
				   msg->width,
				   msg->height,
				   data->width,
				   msg->modulo);
    	    break;
	    
	    
    	default:
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    break;
	    
    } /* switch(msg->pixFmt) */
    
}

/*********  BitMap::PutImageLUT()  ***************************/

VOID MNAME_BM(PutImageLUT)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    struct Box box = {0, 0, 0, 0};

    EnterFunc(bug("VGAGfx.BitMap::PutImageLUT(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	msg->pixels, msg->x, msg->y, msg->width, msg->height));

    HIDD_BM_CopyMemBox8(o,
		    	msg->pixels,
			0,
			0,
			data->VideoData,
			msg->x,
			msg->y,
			msg->width,
			msg->height,
			msg->modulo,
			data->width);
    
    if (data->disp)
    {
        box.x1 = msg->x;
        box.y1 = msg->y;
        box.x2 = box.x1 + msg->width - 1;
        box.y2 = box.y1 + msg->height - 1;

        ObtainSemaphore(&XSD(cl)->HW_acc);

        vgaRefreshArea(data, 1, &box);

        if ( (  (XSD(cl)->mouseX + XSD(cl)->mouseW - 1 >= box.x1) &&
                (XSD(cl)->mouseX <= box.x2) ) ||
            (   (XSD(cl)->mouseY + XSD(cl)->mouseH - 1 >= box.y1) &&
                (XSD(cl)->mouseY <= box.y2) ) )
            draw_mouse(XSD(cl));

        ReleaseSemaphore(&XSD(cl)->HW_acc);

    }
    ReturnVoid("VGAGfx.BitMap::PutImageLUT");
}

/*********  BitMap::GetImageLUT()  ***************************/

VOID MNAME_BM(GetImageLUT)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    HIDD_BM_CopyMemBox8(o,
			data->VideoData,
			msg->x,
			msg->y,
			msg->pixels,
			0,
			0,
			msg->width,
			msg->height,
			data->width,
			msg->modulo);

}

/*********  BitMap::FillRect()  ***************************/

VOID MNAME_BM(FillRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data =OOP_INST_DATA(cl, o);
    struct Box box = {0, 0, 0, 0};
    HIDDT_Pixel fg = GC_FG(msg->gc);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);

    EnterFunc(bug("VGAGfx.BitMap::FillRect(%d,%d,%d,%d)\n",
    	msg->minX, msg->minY, msg->maxX, msg->maxY));

    switch(mode)
    {
        case vHidd_GC_DrawMode_Copy:
	    HIDD_BM_FillMemRect8(o,
	    	    	    	 data->VideoData,
	    	    	    	 msg->minX,
				 msg->minY,
				 msg->maxX,
				 msg->maxY,
				 data->width,
				 fg);
	    break;
	    
	case vHidd_GC_DrawMode_Invert:
	    HIDD_BM_InvertMemRect(o,
	    	    	    	 data->VideoData,
	    	    	    	 msg->minX,
				 msg->minY,
				 msg->maxX,
				 msg->maxY,
				 data->width);
	    break;
	    
	default:
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    break;
	    
    } /* switch(mode) */

    
    if (data->disp)
    {
        box.x1 = msg->minX;
        box.y1 = msg->minY;
        box.x2 = msg->maxX;
        box.y2 = msg->maxY;

        ObtainSemaphore(&XSD(cl)->HW_acc);

        vgaRefreshArea(data, 1, &box);
        if ( (  (XSD(cl)->mouseX + XSD(cl)->mouseW - 1 >= box.x1) &&
                (XSD(cl)->mouseX <= box.x2) ) ||
            (   (XSD(cl)->mouseY + XSD(cl)->mouseH - 1 >= box.y1) &&
                (XSD(cl)->mouseY <= box.y2) ) )
            draw_mouse(XSD(cl));

        ReleaseSemaphore(&XSD(cl)->HW_acc);


    }
    ReturnVoid("VGAGfx.BitMap::FillRect");
}

/*** BitMap::BlitColorExpansion() **********************************************/
VOID MNAME_BM(BlitColorExpansion)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    ULONG cemd;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    struct Box box;
    HIDDT_Pixel fg, bg;
    LONG x, y;

    EnterFunc(bug("VGAGfx.BitMap::BlitColorExpansion(%p, %d, %d, %d, %d, %d, %d)\n"
    	, msg->srcBitMap, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
    
    fg = GC_FG(msg->gc);
    bg = GC_BG(msg->gc);
    cemd = GC_COLEXP(msg->gc);

    if (cemd & vHidd_GC_ColExp_Opaque)
    {
	for (y = 0; y < msg->height; y ++)
	{
            for (x = 0; x < msg->width; x ++)
            {
		ULONG is_set;

		is_set = HIDD_BM_GetPixel(msg->srcBitMap, x + msg->srcX, y + msg->srcY);

   	    	*(data->VideoData + x + msg->destX + ((y + msg->destY) * data->width)) = is_set ? fg : bg;

	    } /* for (each x) */

	} /* for (each y) */
    	
    }
    else
    {
	for (y = 0; y < msg->height; y ++)
	{
            for (x = 0; x < msg->width; x ++)
            {
		ULONG is_set;

		is_set = HIDD_BM_GetPixel(msg->srcBitMap, x + msg->srcX, y + msg->srcY);

    	    	if (is_set)
   	    	    *(data->VideoData + x + msg->destX + ((y + msg->destY) * data->width)) = fg;

	    } /* for (each x) */

	} /* for (each y) */
    }

    if (data->disp)
    {
        box.x1 = msg->destX;
        box.y1 = msg->destY;
        box.x2 = box.x1 + msg->width - 1;
        box.y2 = box.y1 + msg->height - 1;

        ObtainSemaphore(&XSD(cl)->HW_acc);

        vgaRefreshArea(data, 1, &box);
        if ( (  (XSD(cl)->mouseX + XSD(cl)->mouseW - 1 >= box.x1) &&
                (XSD(cl)->mouseX <= box.x2) ) ||
            (   (XSD(cl)->mouseY + XSD(cl)->mouseH - 1 >= box.y1) &&
                (XSD(cl)->mouseY <= box.y2) ) )
            draw_mouse(XSD(cl));

        ReleaseSemaphore(&XSD(cl)->HW_acc);

    }    
    ReturnVoid("VGAGfx.BitMap::BlitColorExpansion");
}

/*** BitMap::Set() *******************************************/

VOID MNAME_ROOT(Set)(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;

    tstate = msg->attrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
            case aoHidd_BitMap_Visible:
		data->disp = tag->ti_Data;
		D(bug("[VGAGfx] BitMap::Visible set to %d\n", data->disp));
		if (data->disp) {
		    /* Show the bitmap */
		    struct Box box = {0, 0, data->width-1, data->height-1};

		    /* Turn off text-mode debug console */
		    bug("\x03");

		    ObtainSemaphore(&XSD(cl)->HW_acc);
		    vgaRestore(data->Regs, FALSE);
		    vgaRefreshArea(data, 1, &box);
		    draw_mouse(XSD(cl));
		    ReleaseSemaphore(&XSD(cl)->HW_acc);
		}
		break;
	    }
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*** BitMap::Get() *******************************************/

VOID MNAME_ROOT(Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_VGABM_ATTR(msg->attrID, idx)) {
	switch (idx) {
	case aoHidd_VGABitMap_Drawable:
	    *msg->storage = (IPTR)data->VideoData;
	    return;
	}
    } else if (IS_BM_ATTR(msg->attrID, idx)) {
	switch (idx) {
	case aoHidd_BitMap_Visible:
	    *msg->storage = data->disp;
	    return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

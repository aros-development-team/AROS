/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for VGA hidd.
    Lang: English.
*/

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

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

void vgaRestore(struct vgaHWRec *);
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
	OOP_Object *gfxhidd, *sync, *pf;
	IPTR modeid = vHidd_ModeID_Invalid;
        IPTR width, height, depth;
	IPTR dwidth, dheight;
	IPTR displayable = FALSE;

        data = OOP_INST_DATA(cl, o);

	/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
	/* Get attr values */
	OOP_GetAttr(o, aHidd_BitMap_ModeID     , &modeid     );
	OOP_GetAttr(o, aHidd_BitMap_Width      , &width      );
	OOP_GetAttr(o, aHidd_BitMap_Height     , &height     );
	OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);
	OOP_GetAttr(o, aHidd_BitMap_PixFmt     , (IPTR *)&pf );
	D(bug("[VGABitMap] PixFmt object: 0x%p\n", pf));
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

	D(bug("[VGABitMap] Size: %lux%lu, depth: %lu\n", width, height, depth));
	D(bug("[VGABitMap] Displayable: %ld\n", displayable));
	ASSERT (width != 0 && height != 0 && depth != 0);

	data->width  = width;
	data->height = height;
	data->bpp    = depth;
	
	if (modeid != vHidd_ModeID_Invalid) {
	    
	    OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
	    HIDD_Gfx_GetMode(gfxhidd, modeid, &sync, &pf);
	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &dwidth);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &dheight);
	    data->disp_width  = dwidth;
	    data->disp_height = dheight;
	    D(bug("[VGABitMap] Display size: %dx%d\n", dwidth, dheight));
	}

	width=(width+15) & ~15;
	data->bpr = width;
	data->VideoData = AllocVec(width*height,MEMF_PUBLIC|MEMF_CLEAR);
	D(bug("[VGABitMap] Allocated videodata at 0x%p\n", data->VideoData));
	if (data->VideoData) {
            struct TagItem tags[2];

            tags[0].ti_Tag = aHidd_ChunkyBM_Buffer;
            tags[0].ti_Data = (IPTR)data->VideoData;
            tags[1].ti_Tag = TAG_END;
            OOP_SetAttrs(o, tags);

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
		IPTR pixelc;
		
		/* We should have got modeID from the bitmap */				
		if (modeid != vHidd_ModeID_Invalid)
		{
		    mode.Width 	= dwidth;
		    mode.Height = dheight;
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
    
    FreeVec(data->VideoData);
    FreeVec(data->Regs);
    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("VGAGfx.BitMap::Dispose");
}

/*********  BitMap::SetColors()  ***************************/

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
// FIXME: in theory we shouldn't need this method since the superclass implements it

VOID MNAME_BM(PutPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    return;
}

/*** BitMap::Set() *******************************************/

VOID MNAME_ROOT(Set)(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;
    int limit;
    int xoffset = data->xoffset;
    int yoffset = data->yoffset;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
            case aoHidd_BitMap_Visible:
		data->disp = tag->ti_Data;
		D(bug("[VGAGfx] BitMap::Visible set to %d\n", data->disp));
		if (data->disp) {
		    struct Box box;

		    /* Program video mode on the card */
		    bug("\x03");
		    vgaRestore(data->Regs);
		    /* If our bitmap is smaller than display area,
		       we need to clear two rectangles: to the right
		       and to the bottom of the used framebuffer
		       portion */
		    if (data->disp_width > data->width) {
			box.x1 = data->width;
			box.y1 = 0;
			box.x2 = data->disp_width - 1;
			box.y2 = data->height - 1;
			vgaEraseArea(data, &box);
		    }
		    if (data->disp_height > data->height) {
			box.x1 = 0;
			box.y1 = data->height;
			box.x2 = data->width - 1;
			box.y2 = data->disp_height - 1;
			vgaEraseArea(data, &box);
		    }
		}
		break;
	    case aoHidd_BitMap_LeftEdge:
	        xoffset = tag->ti_Data;
    		limit = data->disp_width - data->width;
    		if (xoffset > 0)
		    xoffset = 0;
		else if (xoffset < limit)
		    xoffset = limit;
		break;
	    case aoHidd_BitMap_TopEdge:
	        yoffset = tag->ti_Data;
		limit = data->disp_height - data->height;
		if (yoffset > 0)
		    yoffset = 0;
		else if (yoffset < limit)
		    yoffset = limit;
		break;
	    }
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if ((xoffset != data->xoffset) || (yoffset != data->yoffset)) {
	data->xoffset = xoffset;
	data->yoffset = yoffset;
	    
	if (data->disp) {
	    struct Box box = {0, 0, data->disp_width - 1, data->disp_height - 1};

    	    ObtainSemaphore(&XSD(cl)->HW_acc);
	    vgaRefreshArea(data, &box);
            draw_mouse(XSD(cl));
    	    ReleaseSemaphore(&XSD(cl)->HW_acc);
	}
    }
}

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
	case aoHidd_BitMap_LeftEdge:
	    *msg->storage = data->xoffset;
	    return;
	case aoHidd_BitMap_TopEdge:
	    *msg->storage = data->yoffset;
	    return;
	case aoHidd_BitMap_Visible:
	    *msg->storage = data->disp;
	    return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*** BitMap::UpdateRect() *******************************************/

VOID PCVGABM__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    int left = msg->x + data->xoffset;
    int top = msg->y + data->yoffset;
    int right = left + msg->width - 1;
    int bottom = top + msg->height - 1;

    if ((right < 0) || (bottom < 0))
        return;
    if (left < 0)
        left = 0;
    if (top < 0)
        top = 0;

    if (data->disp)
    {
        ObtainSemaphore(&XSD(cl)->HW_acc);
	
	if ((msg->width == 1) && (msg->height == 1))
	    vgaRefreshPixel(data, left, top);
	else {
	    struct Box box = {left, top, right, bottom};

            vgaRefreshArea(data, &box);
	}

        if ( (  (XSD(cl)->mouseX + XSD(cl)->mouseW - 1 >= left) &&
                (XSD(cl)->mouseX <= right) ) ||
            (   (XSD(cl)->mouseY + XSD(cl)->mouseH - 1 >= top) &&
                (XSD(cl)->mouseY <= bottom) ) )
            draw_mouse(XSD(cl));

        ReleaseSemaphore(&XSD(cl)->HW_acc);
    }
}

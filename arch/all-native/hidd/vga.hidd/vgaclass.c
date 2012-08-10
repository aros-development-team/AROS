/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Class for VGA and compatible cards.
    Lang: English.
*/

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include <aros/symbolsets.h>

#include <hardware/custom.h>

#include <devices/inputevent.h>
#include <string.h>

#include "vga.h"
#include "vgaclass.h"
#include "bitmap.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

static AROS_UFIH1(ResetHandler, struct vga_staticdata *, xsd)
{
    AROS_USERFUNC_INIT

/* On my machine this fills the screen with colorful vertical stripes
   instead of blanking. So for now we use software method.
	Pavel Fedin.
    vgaBlankScreen(0); */

    struct bitmap_data *data = OOP_INST_DATA(xsd->bmclass, xsd->visible);

    if (data)
    {
    	struct Box box = {0, 0, data->width - 1, data->height - 1};
	
	vgaEraseArea(data, &box);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

/* Default graphics modes */

struct vgaModeDesc
    vgaDefMode[NUM_MODES]={
		{"640x480x4 @ 60Hz",	// h: 31.5 kHz v: 60Hz
		640,480,4,0,
		0,
		640,664,760,800,0,
		480,491,493,525}
#ifndef ONLY640 
	       ,{"768x576x4 @ 54Hz",	// h: 32.5 kHz v: 54Hz
		768,576,4,1,
		0,
		768,795,805,872,0,
		576,577,579,600},
		{"800x600x4 @ 52Hz",	// h: 31.5 kHz v: 52Hz
		800,600,4,1,
		0,
		800,826,838,900,0,	// 900
		600,601,603,617}	// 617
#endif
		};

/*********************
**  GfxHidd::New()  **
*********************/

#define NUM_SYNC_TAGS 11
#define SET_SYNC_TAG(taglist, idx, tag, val) 	\
    taglist[idx].ti_Tag  = aHidd_Sync_ ## tag;	\
    taglist[idx].ti_Data = val

VOID init_sync_tags(struct TagItem *tags, struct vgaModeDesc *md, STRPTR name)
{
    ULONG clock = (md->clock == 1) ? 28322000 : 25175000;

    SET_SYNC_TAG(tags, 0, PixelClock, 	clock	);
    SET_SYNC_TAG(tags, 1, HDisp, 	md->HDisplay	);
    SET_SYNC_TAG(tags, 2, VDisp, 	md->VDisplay	);
    SET_SYNC_TAG(tags, 3, HSyncStart, 	md->HSyncStart	);
    SET_SYNC_TAG(tags, 4, HSyncEnd, 	md->HSyncEnd	);
    SET_SYNC_TAG(tags, 5, HTotal, 	md->HTotal	);
    SET_SYNC_TAG(tags, 6, VSyncStart,	md->VSyncStart	);
    SET_SYNC_TAG(tags, 7, VSyncEnd, 	md->VSyncEnd	);
    SET_SYNC_TAG(tags, 8, VTotal, 	md->VTotal	);
    SET_SYNC_TAG(tags, 9, Description,  (IPTR)name  	);
    tags[10].ti_Tag = TAG_DONE;
}

OOP_Object *PCVGA__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem pftags[] = {
    	{ aHidd_PixFmt_RedShift     , 0			      }, /* 0 */
	{ aHidd_PixFmt_GreenShift   , 0			      }, /* 1 */
	{ aHidd_PixFmt_BlueShift    , 0			      }, /* 2 */
	{ aHidd_PixFmt_AlphaShift   , 0			      }, /* 3 */
	{ aHidd_PixFmt_RedMask      , 0x000000FC	      }, /* 4 */
	{ aHidd_PixFmt_GreenMask    , 0x0000FC00	      }, /* 5 */
	{ aHidd_PixFmt_BlueMask     , 0x00FC0000	      }, /* 6 */
	{ aHidd_PixFmt_AlphaMask    , 0x00000000	      }, /* 7 */
	{ aHidd_PixFmt_ColorModel   , vHidd_ColorModel_Palette}, /* 8 */
	{ aHidd_PixFmt_Depth	    , 4			      }, /* 9 */
	{ aHidd_PixFmt_BytesPerPixel, 1			      }, /* 10 */
	{ aHidd_PixFmt_BitsPerPixel , 4			      }, /* 11 */
	{ aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_LUT8    }, /* 12 */
	{ aHidd_PixFmt_CLUTShift    , 0			      }, /* 13 */
	{ aHidd_PixFmt_CLUTMask	    , 0x0f		      }, /* 14 */
	{ aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Chunky }, /* 15 */
	{ TAG_DONE		    , 0UL		      }
    };

    struct TagItem sync_640_480[NUM_SYNC_TAGS];
#ifndef ONLY640 
    struct TagItem sync_758_576[NUM_SYNC_TAGS];
    struct TagItem sync_800_600[NUM_SYNC_TAGS];
#endif

    struct TagItem modetags[] = {
	{ aHidd_Sync_HMax     , 16384			},
	{ aHidd_Sync_VMax     , 16384			},
	{ aHidd_Gfx_PixFmtTags,	(IPTR)pftags		},
	{ aHidd_Gfx_SyncTags  ,	(IPTR)sync_640_480	},
#ifndef ONLY640
	{ aHidd_Gfx_SyncTags  ,	(IPTR)sync_758_576	},
	{ aHidd_Gfx_SyncTags  ,	(IPTR)sync_800_600	},
#endif
	{ TAG_DONE, 0UL }
    };
    
    struct TagItem mytags[] = {
	{ aHidd_Gfx_ModeTags,	(IPTR)modetags	},
	{ TAG_MORE, 0UL }
    };
    struct pRoot_New mymsg;

    /* Do not allow to create more than one object */
    if (XSD(cl)->vgahidd)
	return NULL;

    /* First init the sync tags */
    init_sync_tags(sync_640_480, &vgaDefMode[0], "VGA:640x480");
#ifndef ONLY640
    init_sync_tags(sync_758_576, &vgaDefMode[1], "VGA:758x576");
    init_sync_tags(sync_800_600, &vgaDefMode[2], "VGA:800x600");
#endif
    
    /* init mytags. We use TAG_MORE to attach our own tags before we send them
    to the superclass */
    mytags[1].ti_Tag  = TAG_MORE;
    mytags[1].ti_Data = (IPTR)msg->attrList;

    /* Init mymsg. We have to use our own message struct because
       one should not alter the one passed to this method.
       message structs passed to a method are always read-only.
       (The user who called us might want to reuse the same msg struct
       for several calls, but that will break if some method changes the
       msg struct contents)
    */
    mymsg.mID	= msg->mID;	/* We got New() method and we are sending 
				   the same method to the superclass	*/
    mymsg.attrList = mytags;
    msg = &mymsg;

    EnterFunc(bug("VGAGfx::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    XSD(cl)->vgahidd = o;
    if (o) {
        struct Vga_Data *data = OOP_INST_DATA(cl, o);

	data->ResetInterrupt.is_Code = (VOID_FUNC)ResetHandler;
	data->ResetInterrupt.is_Data = XSD(cl);
	AddResetCallback(&data->ResetInterrupt);
    }
    ReturnPtr("VGAGfx::New", OOP_Object *, o);
}

VOID PCVGA__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct Vga_Data *data = OOP_INST_DATA(cl, o);

    RemResetCallback(&data->ResetInterrupt);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    XSD(cl)->vgahidd = NULL;
}

VOID PCVGA__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    BOOL found = FALSE;
    if (IS_GFX_ATTR(msg->attrID, idx)) {
    	switch (idx) {
	     case aoHidd_Gfx_SupportsHWCursor:
	     case aoHidd_Gfx_NoFrameBuffer:
	     	*msg->storage = (IPTR)TRUE;
		found = TRUE;
		break;
	}
    }
    
    if (!found)
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	
    return;
}

/********** GfxHidd::NewBitMap()  ****************************/
OOP_Object *PCVGA__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    struct TagItem mytags[2];
    struct pHidd_Gfx_NewBitMap mymsg;
    HIDDT_ModeID modeid;
    
    EnterFunc(bug("VGAGfx::NewBitMap()\n"));

    modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    if (vHidd_ModeID_Invalid != modeid) {
	/* User supplied a valid modeid. We can use our class */
	mytags[0].ti_Tag	= aHidd_BitMap_ClassPtr;
	mytags[0].ti_Data	= (IPTR)XSD(cl)->bmclass;
	mytags[1].ti_Tag	= TAG_MORE;
	mytags[1].ti_Data	= (IPTR)msg->attrList;
	/* Like in Gfx::New() we init a new message struct */
	mymsg.mID	= msg->mID;
	mymsg.attrList	= mytags;
	/* Pass the new message to the superclass */
	msg = &mymsg;
    }

    ReturnPtr("VGAGfx::NewBitMap", OOP_Object *, (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg));
}

/*********  GfxHidd::Show()  ***************************/

OOP_Object *PCVGA__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    /* We currently use class static data instead of
       object data. In addition we directly access
       bitmap's private data. This is horribly wrong
       and needs further refactoring */
    struct vga_staticdata *data = XSD(cl);
    struct Box box;

    D(bug("[VGAGfx] Show(0x%p)\n", msg->bitMap));
    ObtainSemaphore(&data->sema);

    /* Remove old bitmap from the screen */
    if (data->visible) {
	IPTR tags[] = {aHidd_BitMap_Visible, FALSE, TAG_DONE};

	D(bug("[VGAGfx] Old displayed bitmap: 0x%p\n", data->visible));
	OOP_SetAttrs(data->visible, (struct TagItem *)tags);
    }

    if (msg->bitMap) {
	/* If we have a bitmap to show, set it as visible */
	IPTR tags[] = {aHidd_BitMap_Visible, TRUE, TAG_DONE};
	OOP_Object *pixfmt;
	IPTR depth;

	OOP_GetAttr(msg->bitMap, aHidd_BitMap_PixFmt, (IPTR *)&pixfmt);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth, &depth);
	/* TODO: this should be brought in from SpriteBase of the colormap */
	data->mouseBase = (depth > 4) ? 16 : (1 << depth) - 8;

	OOP_SetAttrs(msg->bitMap, (struct TagItem *)tags);
	data->visible = msg->bitMap;
    } else {
	/* Otherwise simply clear the framebuffer */
	box.x1 = 0;
	box.y1 = 0;
	box.x2 = 639;
	box.y2 = 479;
	ObtainSemaphore(&data->HW_acc);
	/* We use old visible bitmap pointer here since this bitmap
	   contains data about the current video mode */
        vgaEraseArea(OOP_INST_DATA(data->bmclass, data->visible), &box);
	draw_mouse(data);
	ReleaseSemaphore(&data->HW_acc);

	data->visible = NULL;
    }
    D(bug("[VGAGfx] New displayed bitmap: 0x%p\n", data->visible));
    D(bug("[VGAGfx] Mouse pointer base color: %u\n", data->mouseBase));
    
    ReleaseSemaphore(&data->sema);
    return msg->bitMap;
}

/*********  GfxHidd::CopyBox()  ***************************/

VOID PCVGA__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    ULONG mode;
    unsigned char *src = 0, *dest = 0;

    mode = GC_DRMD(msg->gc);

    EnterFunc(bug("VGAGfx.BitMap::CopyBox (%d,%d) to (%d,%d) of dim %d,%d\n",
    	msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
    D(bug("[VGAGfx] Src: 0x%p, dest: 0x%p\n", msg->src, msg->dest));
    OOP_GetAttr(msg->src,  aHidd_VGABitMap_Drawable, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_VGABitMap_Drawable, (IPTR *)&dest);

    if (!dest || !src ||
    	((mode != vHidd_GC_DrawMode_Copy) &&
	 (mode != vHidd_GC_DrawMode_And) &&
	 (mode != vHidd_GC_DrawMode_Xor) &&
	 (mode != vHidd_GC_DrawMode_Clear) &&
	 (mode != vHidd_GC_DrawMode_Invert)))
    {
	/* The source and/or destination object is no VGA bitmap, onscreen nor offscreen.
	   Or drawmode is not one of those we accelerate. Let the superclass do the
	   copying in a more general way
	*/
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	return;
	
    }

    {
    	struct bitmap_data *data = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        struct bitmap_data *ddata = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
        int i, width, phase, j;
	BOOL descending;

        // start of Source data
        unsigned char *s_start = data->VideoData +
                                 msg->srcX + (msg->srcY * data->bpr);
        // adder for each line
        ULONG s_add = data->bpr - msg->width;
        ULONG cnt = msg->height;

        unsigned char *d_start = ddata->VideoData +
                                 msg->destX + (msg->destY * ddata->bpr);
        ULONG d_add = ddata->bpr - msg->width;

	width = msg->width;

    	if ((msg->srcY > msg->destY) || ((msg->srcY == msg->destY) && (msg->srcX >= msg->destX)))
	{
	    if ((phase = ((IPTR)s_start & 3L)))
	    {
		phase = 4 - phase;
		if (phase > width) phase = width;
		width -= phase;
	    }
	    descending = FALSE;
	}
	else
	{
	    s_start += (cnt - 1) * data->bpr + width;
	    d_start += (cnt - 1) * ddata->bpr + width;

	    phase = ((IPTR)s_start & 3L);
	    if (phase > width) phase = width;
	    width -= phase;
	    
	    descending = TRUE;
	}

        switch(mode)
	{
	    case vHidd_GC_DrawMode_Copy:
	    	HIDD_BM_CopyMemBox8(msg->dest,
		    	    	    data->VideoData,
				    msg->srcX,
				    msg->srcY,
				    ddata->VideoData,
				    msg->destX,
				    msg->destY,
				    msg->width,
				    msg->height,
				    data->bpr,
				    ddata->bpr);
		break;
		
	    case vHidd_GC_DrawMode_And:
	    	if (!descending)
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *d_start++ &= *s_start++;
                	}
	        	while (i >= 4)
	        	{
		            *((ULONG*)d_start) &= *((ULONG*)s_start);
		            d_start += 4;
		            s_start += 4;
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *d_start++ &= *s_start++;
                	}
                	d_start += d_add;
                	s_start += s_add;
                    }
		}
		else
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *--d_start &= *--s_start;
                	}
	        	while (i >= 4)
	        	{
		            d_start -= 4;
		            s_start -= 4;
		            *((ULONG*)d_start) &= *((ULONG*)s_start);
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *--d_start &= *--s_start;
                	}
                	d_start -= d_add;
                	s_start -= s_add;
                    }
		}
		
		break;

	    case vHidd_GC_DrawMode_Xor:
	    	if (!descending)
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *d_start++ ^= *s_start++;
                	}
	        	while (i >= 4)
	        	{
		            *((ULONG*)d_start) ^= *((ULONG*)s_start);
		            d_start += 4;
		            s_start += 4;
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *d_start++ ^= *s_start++;
                	}
                	d_start += d_add;
                	s_start += s_add;
                    }
		}
		else
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *--d_start ^= *--s_start;
                	}
	        	while (i >= 4)
	        	{
		            d_start -= 4;
		            s_start -= 4;
		            *((ULONG*)d_start) ^= *((ULONG*)s_start);
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *--d_start ^= *--s_start;
                	}
                	d_start -= d_add;
                	s_start -= s_add;
                    }
		}
		break;
	    	
	    case vHidd_GC_DrawMode_Clear:
	    	if (!descending)
		{		
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *d_start++ = 0;
                	}
	        	while (i >= 4)
	        	{
		            *((ULONG*)d_start) = 0;
		            d_start += 4;
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *d_start++ = 0;
                	}
                	d_start += d_add;
                    }
		}
		else
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *--d_start = 0;
                	}
	        	while (i >= 4)
	        	{
		            d_start -= 4;
		            *((ULONG*)d_start) = 0;
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *--d_start = 0;
                	}
                	d_start -= d_add;
                    }
		}
    	    	break;
			    	
	    case vHidd_GC_DrawMode_Invert:
	    	if (!descending)
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *d_start = ~*d_start;
			    d_start++;
                	}
	        	while (i >= 4)
	        	{
		            *((ULONG*)d_start) = ~*((ULONG*)d_start);
		            d_start += 4;
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *d_start = ~*d_start;
			    d_start++;
                	}
                	d_start += d_add;
                    }
		}
		else
		{
                    while (cnt--)
    	            {
	        	i = width;
	        	j = phase;
                	while (j--)
                	{
                            *d_start = ~*d_start;
			    d_start--;
                	}
	        	while (i >= 4)
	        	{
		            d_start -= 4;
		            *((ULONG*)d_start) = ~*((ULONG*)d_start);
		            i -= 4;
	        	}
	        	while (i--)
                	{
                            *d_start = ~*d_start;
			    d_start--;
                	}
                	d_start -= d_add;
                    }
		    break;
		}
		break;
		
	} /* switch(mode) */
    }
    ReturnVoid("VGAGfx.BitMap::CopyBox");
}

/********** GfxHidd::SetCursorShape()  ****************************/

BOOL PCVGA__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct vga_staticdata *data = XSD(cl);
    IPTR curs_width, curs_height;
    UBYTE *new_curs_pixels;

    OOP_GetAttr(msg->shape, aHidd_BitMap_Width,  &curs_width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &curs_height);

    new_curs_pixels = AllocMem(curs_width * curs_height, MEMF_ANY);
    if (!new_curs_pixels)
	return FALSE;

    HIDD_BM_GetImage(msg->shape, new_curs_pixels, curs_width, 0, 0, curs_width, curs_height, vHidd_StdPixFmt_LUT8);

    ObtainSemaphore(&data->HW_acc);
    erase_mouse(data);
    if (data->mouseShape)
	FreeMem(data->mouseShape, data->mouseW * data->mouseH);

    data->mouseW = curs_width;
    data->mouseH = curs_height;
    data->mouseShape = new_curs_pixels;
    draw_mouse(data);

    ReleaseSemaphore(&data->HW_acc);
    return TRUE;
}

/********** GfxHidd::SetCursorPos()  ****************************/

BOOL PCVGA__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    ObtainSemaphore(&XSD(cl)->HW_acc);

    erase_mouse(XSD(cl));

    XSD(cl)->mouseX = (short)msg->x;
    XSD(cl)->mouseY = (short)msg->y;

    if (XSD(cl)->visible)
    {
        struct bitmap_data *bm_data =
            OOP_INST_DATA(XSD(cl)->bmclass, XSD(cl)->visible);

        if (XSD(cl)->mouseX < 0) XSD(cl)->mouseX = 0;
	if (XSD(cl)->mouseY < 0) XSD(cl)->mouseY = 0;
	if (XSD(cl)->mouseX >= bm_data->width) XSD(cl)->mouseX =
            bm_data->width - 1;
	if (XSD(cl)->mouseY >= bm_data->height) XSD(cl)->mouseY =
            bm_data->height - 1;
    }
    
    draw_mouse(XSD(cl));

    ReleaseSemaphore(&XSD(cl)->HW_acc);
    
    return TRUE;
}

/********** GfxHidd::SetCursorVisible()  ****************************/

VOID PCVGA__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    XSD(cl)->mouseVisible = msg->visible;

    ObtainSemaphore(&XSD(cl)->HW_acc);
    erase_mouse(XSD(cl));
    draw_mouse(XSD(cl));
    ReleaseSemaphore(&XSD(cl)->HW_acc);
}

/* end of stuff added by stegerg */
/*******************************************************************/
	       
void draw_mouse(struct vga_staticdata *xsd)
{
    int pix;
    unsigned char *ptr, *data;
    int x, y, width, fg, x_i, y_i;

    if (!xsd->mouseShape)
	return;

    if (xsd->mouseVisible)
    {
        if (xsd->visible)
	{
            struct bitmap_data *bm_data =
                OOP_INST_DATA(xsd->bmclass, xsd->visible);

	    /* Get display width */
	    width = bm_data->disp_width;

    	    /* And pointer data */
    	    data = xsd->mouseShape;
    
    	    ObtainSemaphore(&xsd->HW_acc);

    	    outw(0x3c4,0x0f02);
	    outw(0x3ce,0x0005);
	    outw(0x3ce,0x0003);
	    outw(0x3ce,0x0f01);

	    for (y_i = 0, y = xsd->mouseY ; y_i < xsd->mouseH; y_i++, y++)
    	    {
    		for (x_i = 0, x = xsd->mouseX; x_i < xsd->mouseW; x_i++, x++)
		{
		    ptr = (char *)(IPTR)(0xa0000 + (x + (y * width)) / 8);
		    pix = 0x8000 >> (x % 8);
    
		    fg = (char)*data++;

		    if (fg && (x < width))
		    {
			fg += xsd->mouseBase;
			outw(0x3ce,pix | 8);
			outw(0x3ce,(fg << 8));

			*ptr |= 1;		// This or'ed value isn't important
		    }
		}
	    }
	    
	    ReleaseSemaphore(&xsd->HW_acc);
	}
    }
}

void erase_mouse(struct vga_staticdata *data)
{
    if (data->visible) {
        struct Box box = {0, 0, 0, 0};

	box.x1 = data->mouseX;
        box.y1 = data->mouseY;
        box.x2 = box.x1 + data->mouseW;
        box.y2 = box.y1 + data->mouseH;

	vgaRefreshArea(OOP_INST_DATA(data->bmclass, data->visible), &box);
    }
}

/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for native Amiga chipset.
    Lang: English.
    
*/

/****************************************************************************************/

#define __OOP_NOATTRBASES__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/alerts.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>
#include <hidd/graphics.h>
#include <aros/symbolsets.h>

#define DEBUG 0
#define DB2(x) ;
#define DEBUG_TEXT(x)
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "amigavideogfx.h"
#include "amigavideobitmap.h"

#include "chipset.h"
#include "blitter.h"

/****************************************************************************************/

#define AO(x) 	    	  (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)

/****************************************************************************************/

OOP_Object *AmigaVideoBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    IPTR width, height, depth;
    BOOL  ok = TRUE;      
    struct amigabm_data *data;
    OOP_Object      	 *pf;
    APTR		 p_pf = &pf;
    ULONG		 align, bmadd;

    DB2(bug("AmigaVideoBM__Root__New\n"));

    o =(OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
    	return NULL;
	
    data = OOP_INST_DATA(cl, o);
    memset(data, 0, sizeof  (*data));

    /* Get some data about the dimensions of the bitmap */
    data->planes_alloced = (BOOL)GetTagData(aHidd_PlanarBM_AllocPlanes, TRUE, msg->attrList);
    align = csd->aga ? 63 : 15; // AGA 64-bit fetchmode needs 8-byte alignment
    bmadd = csd->aga ? 16 : 0;
    
    /* FIXME: Fix this hack */
    /* Because this class is used to emulate Amiga bitmaps, we
       have to see if it should have late initalisation
    */
    if (!data->planes_alloced)
	return o; /* Late initialization */

    /* Not late initalization. Get some info on the bitmap */	
    OOP_GetAttr(o, aHidd_BitMap_Width,	&width);
    OOP_GetAttr(o, aHidd_BitMap_Height,	&height);
    OOP_GetAttr(o, aHidd_BitMap_Depth, &depth);

    DB2(bug("%dx%dx%d\n", width, height, depth));

    /* We cache some info */
    data->width = width;
    data->bytesperrow = ((width + align) & ~align) / 8;
    data->height = height;
    data->depth = depth;

    if (ok) {
	/* Allocate memory for plane array */
	data->planes = AllocVec(sizeof(UBYTE*) * depth, MEMF_PUBLIC | MEMF_CLEAR);
	if (NULL == data->planes) {
	    ok = FALSE;
	} else {
	    UBYTE i;
	    /* Allocate all the planes */
	    for (i = 0; i < depth && ok; i++) {
	    	data->planes[i] = AllocMem(data->bytesperrow * data->height, MEMF_CHIP | MEMF_CLEAR);
	    	if (NULL == data->planes[i])
	    	    ok = FALSE;
	    }
	}
    }
      
    if (!ok) {
 	OOP_MethodID dispose_mid;
	    
	dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
		
	o = NULL;
    }
    
    DB2(bug("ret=%x bm=%x\n", o, data));
  	
    return o;
}

VOID AmigaVideoBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct amigabm_data    *data;
    UBYTE   	    	    i;
    
    data = OOP_INST_DATA(cl, o);
    
    DB2(bug("AmigaVideoBM__Root__Dispose %x bm=%x\n", o, data));
    if (data->disp)
    	DB2(bug("removing displayed bitmap?!\n"));
    
    if (data->planes_alloced)
    {
	if (NULL != data->planes)
	{
	    for (i = 0; i < data->depth; i ++)
	    {
		if (NULL != data->planes[i])
		{
		    FreeMem(data->planes[i], data->bytesperrow * data->height);
		}
	    }
	    FreeVec(data->planes);
	}
    }
    
    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}


VOID AmigaVideoBM__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigabm_data *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;
    BOOL moved = FALSE;

    DB2(bug("AmigaVideoBM__Root__Set\n"));
    tstate = msg->attrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
 	DB2(bug("%d/%d\n", tag->ti_Tag, tag->ti_Data));
        if(IS_BITMAP_ATTR(tag->ti_Tag, idx))
        {
            DB2(bug("->%d\n", idx));
            switch(idx)
            {
	        case aoHidd_BitMap_Visible:
	        data->disp = tag->ti_Data;
	        if (data->disp)
	            setmode(csd, data);
		    break;
		case aoHidd_BitMap_LeftEdge:
		    if (data->leftedge != tag->ti_Data) {
		    	data->leftedge = tag->ti_Data;
		    	moved = TRUE;
		    }
		    break;
		case aoHidd_BitMap_TopEdge:
		    if (data->topedge != tag->ti_Data) {
		    	data->topedge = tag->ti_Data;
		    	moved = TRUE;
		    }
		    break;
	    }
	}
    }
    DB2(bug("AmigaVideoBM__Root__Set Exit\n"));
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (moved && csd->disp == data)
    	setscroll(csd, data);
}

VOID AmigaVideoBM__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigabm_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    DB2(bug("AmigaVideoBM__Root__Get %d, Attr=%d AmigaVideoBitmap=%d\n", msg->attrID, __IHidd_Attr, __IHidd_AmigaVideoBitmap));
    if (IS_AmigaVideoBM_ATTR(msg->attrID, idx)) {
 	DB2(bug("AVBM=%d\n", idx));
	switch (idx)
	{
	case aoHidd_AmigaVideoBitMap_Drawable:
	    *msg->storage = TRUE;
	    return;
	}
    } else if (IS_BITMAP_ATTR(msg->attrID, idx)) {
 	DB2(bug("BM=%d\n", idx));
	switch (idx)
	{
	case aoHidd_BitMap_LeftEdge:
	    *msg->storage = data->leftedge;
	    return;
	case aoHidd_BitMap_TopEdge:
	    *msg->storage = data->topedge;
	    return;
	case aoHidd_BitMap_Visible:
	    *msg->storage = data->disp;
	    return;
	case aoHidd_BitMap_Align:
	    *msg->storage = csd->aga ? 64 : 16;
	    return;
	}
    }
    DB2(bug("AmigaVideoBM__Root__Get Exit\n"));
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

static int AmigaVideoBM_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("AmigaVideoBM_Init\n"));
    return TRUE; //return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int AmigaVideoBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("AmigaVideoBM_Expunge\n"));
    //OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(AmigaVideoBM_Init, 0);
ADD2EXPUNGELIB(AmigaVideoBM_Expunge, 0);


BOOL AmigaVideoBM__Hidd_BitMap__SetColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct amigabm_data *data = OOP_INST_DATA(cl, o);
    struct amigavideo_staticdata *csd = CSD(cl);

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg))
    	return FALSE;
    return setcolors(csd, msg, data->disp);
}

VOID AmigaVideoBM__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o,
				struct pHidd_BitMap_PutPixel *msg)
{
    UBYTE   	    	    **plane;
    struct amigabm_data    *data;
    ULONG   	    	    offset;
    ULONG   	    	    mask;
    UBYTE   	    	    pixel, notpixel;
    UBYTE   	    	    i;
    
    data = OOP_INST_DATA(cl, o);

    /* bitmap in plane-mode */
    plane     = data->planes;
    offset    = msg->x / 8 + msg->y * data->bytesperrow;
    pixel     = 128 >> (msg->x % 8);
    notpixel  = ~pixel;
    mask      = 1;

    for(i = 0; i < data->depth; i++, mask <<=1, plane ++)
    {  
    	if ((*plane != NULL) && (*plane != (UBYTE *)-1))
	{	
	    if(msg->pixel & mask)
	    {
		*(*plane + offset) = *(*plane + offset) | pixel;
	    }
	    else
	    {
		*(*plane + offset) = *(*plane + offset) & notpixel;
	    }
        }
    }
    //bug("putpixel %dx%d %x\n", msg->x, msg->y, msg->pixel);
}

/****************************************************************************************/

ULONG AmigaVideoBM__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o,
				struct pHidd_BitMap_GetPixel *msg)
{
    struct amigabm_data    *data;
    UBYTE   	    	    **plane;
    ULONG   	    	    offset;
    ULONG   	    	    i;
    UBYTE   	    	    pixel;
    ULONG   	    	    retval;

    data = OOP_INST_DATA(cl, o);

    plane     = data->planes;
    offset    = msg->x / 8 + msg->y * data->bytesperrow;
    pixel     = 128 >> (msg->x % 8);
    retval    = 0;

    for(i = 0; i < data->depth; i++, plane ++)
    {
    
        if (*plane == (UBYTE *)-1)
	{
	    retval = retval | (1 << i);
	}
	else if (*plane != NULL)
	{	
	    if(*(*plane + offset) & pixel)
	    {
		retval = retval | (1 << i);
	    }
	}
    }
    //bug("getpixel %dx%d=%x\n", msg->x, msg->y, retval);
    return retval; 
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__DrawLine(OOP_Class *cl, OOP_Object *o,
				struct pHidd_BitMap_DrawLine *msg)
{
    HIDDT_Pixel fg = GC_FG(msg->gc);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigabm_data *data = OOP_INST_DATA(cl, o);

    if (msg->x1 == msg->x2 || msg->y1 == msg->y2) {
    	WORD x1 = msg->x1, x2 = msg->x2;
    	WORD y1 = msg->y1, y2 = msg->y2;
    	if (x1 > x2) {
    	    WORD tmp = x1;
    	    x1 = x2;
    	    x2 = tmp;
    	}
    	if (y1 > y2) {
    	    WORD tmp = y1;
    	    y1 = y2;
    	    y2 = tmp;
    	}
	if (!blit_fillrect(csd, data, x1, y1, x2, y2, fg, mode))
    	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    } else {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o,
				struct pHidd_BitMap_PutImage *msg)
{
    WORD    	    	    x, y, d;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    UBYTE   	    	    **plane;
    ULONG   	    	    planeoffset;
    struct amigabm_data    *data = OOP_INST_DATA(cl, o);

    if ((msg->pixFmt != vHidd_StdPixFmt_Native) &&
    	(msg->pixFmt != vHidd_StdPixFmt_Native32))
    {
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	return;
    }
    
    planeoffset = msg->y * data->bytesperrow + msg->x / 8;
    
    for(y = 0; y < msg->height; y++)
    {
    	switch(msg->pixFmt)
	{
	    case vHidd_StdPixFmt_Native:
	    {
	     	UBYTE *src = pixarray;
	
    		plane = data->planes;

    		for(d = 0; d < data->depth; d++)
		{
		    ULONG dmask = 1L << d;
		    ULONG pmask = 0x80 >> (msg->x & 7);
		    UBYTE *pl = *plane;

		    if (pl == (UBYTE *)-1) continue;
		    if (pl == NULL) continue;

		    pl += planeoffset;

    		    for(x = 0; x < msg->width; x++)
		    {
	    		if (src[x] & dmask)
			{
			    *pl |= pmask;
			}
			else
			{
			    *pl &= ~pmask;
			}

			if (pmask == 0x1)
			{
			    pmask = 0x80;
			    pl++;
			}
			else
			{
			    pmask >>= 1;
			}

		    } /* for(x = 0; x < msg->width; x++) */

		    plane++;

		} /* for(d = 0; d < data->depth; d++) */

		pixarray += msg->modulo;
		planeoffset += data->bytesperrow;
	    }
	    break;

	    case vHidd_StdPixFmt_Native32:
	    {
	     	HIDDT_Pixel *src = (HIDDT_Pixel *)pixarray;
	
    		plane = data->planes;

    		for(d = 0; d < data->depth; d++)
		{
		    ULONG dmask = 1L << d;
		    ULONG pmask = 0x80 >> (msg->x & 7);
		    UBYTE *pl = *plane;

		    if (pl == (UBYTE *)-1) continue;
		    if (pl == NULL) continue;

		    pl += planeoffset;

    		    for(x = 0; x < msg->width; x++)
		    {
	    		if (src[x] & dmask)
			{
			    *pl |= pmask;
			}
			else
			{
			    *pl &= ~pmask;
			}

			if (pmask == 0x1)
			{
			    pmask = 0x80;
			    pl++;
			}
			else
			{
			    pmask >>= 1;
			}

		    } /* for(x = 0; x < msg->width; x++) */

		    plane++;

		} /* for(d = 0; d < data->depth; d++) */

		pixarray += msg->modulo;
		planeoffset += data->bytesperrow;
	    }
	    
	    break;
	    
	} /* switch(msg->pixFmt) */    
	
    } /* for(y = 0; y < msg->height; y++) */
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    HIDDT_Pixel fg = GC_FG(msg->gc);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigabm_data *data = OOP_INST_DATA(cl, o);

    if (!blit_fillrect(csd, data, msg->minX, msg->minY, msg->maxX, msg->maxY, fg, mode))
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__PutTemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigabm_data *data = OOP_INST_DATA(cl, o);

    D(bug("puttemplate: %x x=%d y=%d w=%d h=%d srcx=%d modulo=%d invert=%d\n",
    	msg->Template, msg->x, msg->y, msg->width, msg->height, msg->srcx, msg->inverttemplate));
    if (!blit_puttemplate(csd, data, msg))
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}


/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
}

/****************************************************************************************/

BOOL AmigaVideoBM__Hidd_PlanarBM__SetBitMap(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_PlanarBM_SetBitMap *msg)
{
    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

BOOL AmigaVideoBM__Hidd_PlanarBM__GetBitMap(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_PlanarBM_GetBitMap *msg)
{
    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

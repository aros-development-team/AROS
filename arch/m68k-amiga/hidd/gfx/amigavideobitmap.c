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

#define DEBUG 1
#define DEBUG_TEXT(x)
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "amigavideogfx.h"
#include "amigavideobitmap.h"

#include "chipset.h"

/****************************************************************************************/

#define AO(x) 	    	  (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)

/****************************************************************************************/

OOP_Object *AmigaVideoBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    IPTR width, height, depth;
    BOOL  ok = TRUE;      
    struct planarbm_data *data;
    OOP_Object      	 *pf;
    APTR		 p_pf = &pf;
    ULONG		 align;

    bug("AmigaVideoBM__Root__New\n");

    o =(OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o) {
    	bug("super=null\n");
    	return NULL;
    }
	
    data = OOP_INST_DATA(cl, o);
    memset(data, 0, sizeof  (*data));

    /* Get some data about the dimensions of the bitmap */
    data->planes_alloced = (BOOL)GetTagData(aHidd_PlanarBM_AllocPlanes, TRUE, msg->attrList);
    align = GetTagData(aHidd_BitMap_Align, 16, msg->attrList) - 1;
    
    /* FIXME: Fix this hack */
    /* Because this class is used to emulate Amiga bitmaps, we
       have to see if it should have late initalisation
    */
    if (!data->planes_alloced) {
    	bug("late init\n");
	return o; /* Late initialization */
    }

    /* Not late initalization. Get some info on the bitmap */	
    OOP_GetAttr(o, aHidd_BitMap_Width,	&width);
    OOP_GetAttr(o, aHidd_BitMap_Height,	&height);
    OOP_GetAttr(o,  aHidd_BitMap_PixFmt, (IPTR *)p_pf);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, (IPTR *)&depth);
    
    /* We cache some info */
    data->width = width;
    data->bytesperrow = ((width + align) & ~align) / 8;
    data->rows		  = height;
    data->depth		  = depth;

    bug("%dx%dx%d\n", width, height, depth);

    if (ok) {
	/* Allocate memory for plane array */
	data->planes = AllocVec(sizeof (UBYTE *) * depth, MEMF_ANY|MEMF_CLEAR);
	if (NULL == data->planes) {
	    ok = FALSE;
	} else {
	    UBYTE i;
	    /* Allocate all the planes */
	    for (i = 0; i < depth && ok; i++) {
	    	data->planes[i] = AllocVec(height * data->bytesperrow, MEMF_CHIP|MEMF_CLEAR);
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
    
    bug("ret=%x\n", o);
  	
    return o;
}

VOID AmigaVideoBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct planarbm_data    *data;
    UBYTE   	    	    i;
    
    bug("AmigaVideoBM__Root__Dispose\n");
    
    data = OOP_INST_DATA(cl, o);
    
    if (data->planes_alloced)
    {
	if (NULL != data->planes)
	{
	    for (i = 0; i < data->depth; i ++)
	    {
		if (NULL != data->planes[i])
		{
		    FreeVec(data->planes[i]);
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
    struct planarbm_data *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;

    bug("AmigaVideoBM__Root__Set\n");
    tstate = msg->attrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
 	bug("%d/%d\n", tag->ti_Tag, tag->ti_Data);
        if(IS_BITMAP_ATTR(tag->ti_Tag, idx))
        {
            bug("->%d\n", idx);
            switch(idx)
            {
	        case aoHidd_BitMap_Visible:
	        data->disp = tag->ti_Data;
	        if (data->disp)
	            setmode(csd, data);
		    break;
		case aoHidd_BitMap_LeftEdge:
		    break;
		case aoHidd_BitMap_TopEdge:
		    break;
	    }
	}
    }
    bug("AmigaVideoBM__Root__Set Exit\n");
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID AmigaVideoBM__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct planarbm_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    bug("AmigaVideoBM__Root__Get\n");
    if (IS_BITMAP_ATTR(msg->attrID, idx)) {
 	bug("=%d\n", idx);
	switch (idx) {
	case aoHidd_BitMap_LeftEdge:
	    *msg->storage = 0;
	    return;
	case aoHidd_BitMap_TopEdge:
	    *msg->storage = 0;
	    return;
	case aoHidd_BitMap_Visible:
	    *msg->storage = data->disp;
	    return;
	}
    }
    bug("AmigaVideoBM__Root__Get Exit\n");
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

static int AmigaVideoBM_Init(LIBBASETYPEPTR LIBBASE)
{
    bug("AmigaVideoBM_Init\n");
    return TRUE; //return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int AmigaVideoBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    bug("AmigaVideoBM_Expunge\n");
    //OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(AmigaVideoBM_Init, 0);
ADD2EXPUNGELIB(AmigaVideoBM_Expunge, 0);


BOOL AmigaVideoBM__Hidd_BitMap__SetColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct planarbm_data *data = OOP_INST_DATA(cl, o);
    struct amigavideo_staticdata *csd = CSD(cl);
    HIDDT_PixelFormat *pf;

    pf = BM_PIXFMT(o);
    if (    vHidd_ColorModel_StaticPalette == HIDD_PF_COLMODEL(pf)
    	 || vHidd_ColorModel_TrueColor	   == HIDD_PF_COLMODEL(pf) ) {
		return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg))
    	return FALSE;
    return setcolors(csd, msg, data->disp);
}


VOID AmigaVideoBM__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o,
				struct pHidd_BitMap_PutPixel *msg)
{
    UBYTE   	    	    **plane;
    struct planarbm_data    *data;
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
}

/****************************************************************************************/

ULONG AmigaVideoBM__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o,
				 struct pHidd_BitMap_GetPixel *msg)
{
    struct planarbm_data    *data;
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
    
    return retval; 
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o,
				struct pHidd_BitMap_PutImage *msg)
{
    WORD    	    	    x, y, d;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    UBYTE   	    	    **plane;
    ULONG   	    	    planeoffset;
    struct planarbm_data    *data;  

    if ((msg->pixFmt != vHidd_StdPixFmt_Native) &&
    	(msg->pixFmt != vHidd_StdPixFmt_Native32))
    {
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	return;
    }
    
    data = OOP_INST_DATA(cl, o);
    
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

VOID AmigaVideoBM__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_BitMap_PutImageLUT *msg)
{
    WORD    	    	    x, y, d;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    UBYTE   	    	    **plane;
    ULONG   	    	    planeoffset;
    struct planarbm_data   *data;  
    
    data = OOP_INST_DATA(cl, o);
    
    planeoffset = msg->y * data->bytesperrow + msg->x / 8;
    
    for(y = 0; y < msg->height; y++)
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
	
    } /* for(y = 0; y < msg->height; y++) */
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__GetImageLUT(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_BitMap_GetImageLUT *msg)
{
    WORD    	    	    x, y, d;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    UBYTE   	    	    **plane;
    ULONG   	    	    planeoffset;
    struct planarbm_data    *data;  
    UBYTE   	    	    prefill;
    
    data = OOP_INST_DATA(cl, o);
    
    planeoffset = msg->y * data->bytesperrow + msg->x / 8;
    
    prefill = 0;
    for(d = 0; d < data->depth; d++)
    {
    	if (data->planes[d] == (UBYTE *)-1)
	{
	    prefill |= (1L << d);
	}
    }
    
    for(y = 0; y < msg->height; y++)
    {
    	UBYTE *dest = pixarray;
	
    	plane = data->planes;
	
	for(x = 0; x < msg->width; x++)
	{
	    dest[x] = prefill;
	}
	
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
	    	if (*pl & pmask)
		{
		    dest[x] |= dmask;
		}
		else
		{
		    dest[x] &= ~dmask;
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
	
    } /* for(y = 0; y < msg->height; y++) */
    
}

/****************************************************************************************/
#if 0
VOID AmigaVideoBM__Hidd_BitMap__BlitColorExpansion(OOP_Class *cl, OOP_Object *o,
					  struct pHidd_BitMap_BlitColorExpansion *msg)
{
    WORD    	    	    x, y, d;
    UBYTE   	    	    **plane;
    UBYTE   	    	    *mask;
    ULONG   	    	    planeoffset /*, maskoffset*/;
    ULONG   	    	    cemd, fg, bg;
    BOOL    	    	    opaque;
    OOP_Object      	    *gc = msg->gc;
    struct planarbm_data    *data, *maskdata;  

    data     = OOP_INST_DATA(cl, o);

    cemd = GC_COLEXP(gc);
    fg   = GC_FG(gc);
    bg   = GC_BG(gc);
    
    opaque = (cemd & vHidd_GC_ColExp_Opaque) ? TRUE : FALSE;

    planeoffset = msg->destY * data->bytesperrow + msg->destX / 8;

    if (OOP_OCLASS(msg->srcBitMap) == cl)
    {
    	/* srcBitMap is a planarbm class object */
	
	maskdata = OOP_INST_DATA(cl, msg->srcBitMap);
	mask     = maskdata->planes[0];
	mask     += msg->srcY * maskdata->bytesperrow + msg->srcX / 8;

	for(y = 0; y < msg->height; y++)
	{	
    	    plane = data->planes;

    	    for(d = 0; d < data->depth; d++)
	    {
		ULONG dmask = 1L << d;
		ULONG pmask = 0x80 >> (msg->destX & 7);
		ULONG mmask = 0x80 >> (msg->srcX & 7);
    		BOOL  fgset = (fg & dmask) ? TRUE : FALSE;
		BOOL  bgset = (bg & dmask) ? TRUE : FALSE;

		UBYTE *pl = *plane;
    		UBYTE *msk = mask;

		if (pl == (UBYTE *)-1) continue;
		if (pl == NULL) continue;

		pl += planeoffset;

    		for(x = 0; x < msg->width; x++)
		{
	    	    if (*msk & mmask)
		    {
			if (fgset)
		    	    *pl |= pmask;
			else
		    	    *pl &= ~pmask;
		    }
		    else if (opaque)
		    {
			if (bgset)
		    	    *pl |= pmask;
			else
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

		    if (mmask == 0x1)
		    {
			mmask = 0x80;
			msk++;
		    }
		    else
		    {
			mmask >>= 1;
		    }

		} /* for(x = 0; x < msg->width; x++) */
    	    	
		plane++;

	    } /* for(d = 0; d < data->depth; d++) */

	    mask 	+= maskdata->bytesperrow;
	    planeoffset += data->bytesperrow;

	} /* for(y = 0; y < msg->height; y++) */
    
    } /* if (OOP_OCLASS(msg->srcBitMap) == cl) */
    else
    {
    	HIDDT_Pixel *maskline;
	
	maskline = AllocVec(msg->width * sizeof(HIDDT_Pixel), MEMF_PUBLIC);
	if (!maskline)
	{
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    return;
	}
	
	for(y = 0; y < msg->height; y++)
	{	
    	    plane = data->planes;

    	    HIDD_BM_GetImage(msg->srcBitMap,
	    	    	     (UBYTE *)maskline,
			     0,
			     msg->srcX,
			     msg->srcY + y,
			     msg->width,
			     1,
			     vHidd_StdPixFmt_Native32);
			     
    	    for(d = 0; d < data->depth; d++)
	    {
		ULONG dmask = 1L << d;
		ULONG pmask = 0x80 >> (msg->destX & 7);
    		BOOL  fgset = (fg & dmask) ? TRUE : FALSE;
		BOOL  bgset = (bg & dmask) ? TRUE : FALSE;

		UBYTE *pl = *plane;

		if (pl == (UBYTE *)-1) continue;
		if (pl == NULL) continue;

		pl += planeoffset;

    		for(x = 0; x < msg->width; x++)
		{
	    	    if (maskline[x])
		    {
			if (fgset)
		    	    *pl |= pmask;
			else
		    	    *pl &= ~pmask;
		    }
		    else if (opaque)
		    {
			if (bgset)
		    	    *pl |= pmask;
			else
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

	    planeoffset += data->bytesperrow;

	} /* for(y = 0; y < msg->height; y++) */
	
	FreeVec(maskline);
	
    } /* if (OOP_OCLASS(msg->srcBitMap) == cl) else ... */
}
#endif

VOID AmigaVideoBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}


/****************************************************************************************/
#if 0
BOOL AmigaVideoBM__Hidd_PlanarBM__SetBitMap(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_PlanarBM_SetBitMap *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct planarbm_data *data;
    struct BitMap   	 *bm;
    
    struct TagItem  	 pftags[] =
    {
    	{ aHidd_PixFmt_Depth	    , 0UL			},	/* 0 */
    	{ aHidd_PixFmt_BitsPerPixel , 0UL			},	/* 1 */
    	{ aHidd_PixFmt_BytesPerPixel, 1UL			},	/* 2 */
    	{ aHidd_PixFmt_ColorModel   , vHidd_ColorModel_Palette	},	/* 3 */
    	{ aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Planar	},	/* 4 */
    	{ aHidd_PixFmt_CLUTShift    , 0UL			},	/* 5 */
    	{ aHidd_PixFmt_CLUTMask     , 0x000000FF		},	/* 6 */
		{ aHidd_PixFmt_RedMask	    , 0x00FF0000		}, 	/* 7 */
		{ aHidd_PixFmt_GreenMask    , 0x0000FF00		}, 	/* 8 */
		{ aHidd_PixFmt_BlueMask     , 0x000000FF		}, 	/* 9 */
		{ aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_Plane	},
	    { TAG_DONE  	    	    , 0UL   	    	    	}
    };
    struct TagItem  	bmtags[] =
    {
	{ aHidd_BitMap_Width	    , 0UL },
	{ aHidd_BitMap_Height	    , 0UL },
    	{ aHidd_BitMap_PixFmtTags   , 0UL },
	{ TAG_DONE  	    	    , 0UL }
    };
	
    ULONG i;
    
    data = OOP_INST_DATA(cl, o);
    bm = msg->bitMap;
    
    if (data->planes_alloced)
    {
    	D(bug(" !!!!! PlanarBM: Trying to set bitmap in one that allready has planes allocated\n"));
	return FALSE;
    }
    
    /* Check if plane array allready allocated */
    if (NULL != data->planes)
    {
    	if (bm->Depth > data->planebuf_size)
		{
		    FreeVec(data->planes);
		    data->planes = NULL;
		}
    }
    
    if (NULL == data->planes)
    {
	data->planes = AllocVec(sizeof (UBYTE *) * bm->Depth, MEMF_CLEAR);

	if (NULL == data->planes)
	     return FALSE;
	     
	data->planebuf_size = bm->Depth;
    }
    
    
    /* Update the planes */
    for (i = 0; i < data->planebuf_size; i ++)
    {
    	if (i < bm->Depth) 
   	    data->planes[i] = bm->Planes[i];
	else
	    data->planes[i] = NULL;
    }

    data->depth		= bm->Depth;
    data->bytesperrow	= bm->BytesPerRow;
    data->rows		= bm->Rows;
    
    pftags[0].ti_Data = bm->Depth;	/* PixFmt_Depth */
    pftags[1].ti_Data = bm->Depth;	/* PixFmt_BitsPerPixel */
    
    bmtags[0].ti_Data = bm->BytesPerRow * 8;
    bmtags[1].ti_Data = bm->Rows;
    bmtags[2].ti_Data = (IPTR)pftags;
    
    /* Call private bitmap method to update superclass */
    if (!HIDD_BitMap_SetBitMapTags(o, bmtags))
    {
    	ULONG i;
	
	for (i = 0; i < data->planebuf_size; i ++)
	{
	    data->planes[i] = NULL;
	}
    }

    return TRUE;
}

/****************************************************************************************/

BOOL AmigaVideoBM__Hidd_PlanarBM__GetBitMap(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_PlanarBM_GetBitMap *msg)
{
    struct planarbm_data *data = OOP_INST_DATA(cl, o);
    ULONG i;

    msg->bitMap->Depth	     = data->depth;
    msg->bitMap->BytesPerRow = data->bytesperrow;
    msg->bitMap->Rows	     = data->rows;
    msg->bitMap->Flags	     = BMF_STANDARD|BMF_MINPLANES; /* CHECKME */
    msg->bitMap->pad	     = 0;

    for (i = 0; i < data->planebuf_size; i++)
        msg->bitMap->Planes[i] = data->planes[i];

    return TRUE;
}

#endif
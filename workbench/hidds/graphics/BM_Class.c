/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics bitmap class implementation.
    Lang: english
*/

/****************************************************************************************/

#include <string.h>
#include <stdlib.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <oop/oop.h>
#include <graphics/text.h>
#include <graphics/scale.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

#define POINT_OUTSIDE_CLIP(gc, x, y)	\
	(  (x) < GC_CLIPX1(gc)		\
	|| (x) > GC_CLIPX2(gc)		\
	|| (y) < GC_CLIPY1(gc)		\
	|| (y) > GC_CLIPY2(gc) )


    
/* BitMap baseclass is a in C++ terminology a pure virtual
   baseclass. It will not allocate any bitmap data at all,
   that is up to the subclass to do.

   The main task of the BitMap baseclass is to store
   
   There are two ways the we can find out the pixfmt:
   
   Displayable bitmap - 
   	The tags will contrain a modeid.
   	One can use this modeid to get a pointer to an
	allready registered pixfmt.
	
    Non-displayable bitmap -
    	The aHidd_BitMap_StdPixFmt attribute will allways be passed
	  
*/    


/****************************************************************************************/

#define GOT_BM_ATTR(code)   GOT_ATTR(code, aoHidd_BitMap, bitmap)
#define AO(x) 	    	    aoHidd_BitMap_ ## x

#define BMAF(x)     	    (1L << aoHidd_BitMap_ ## x)

#define BM_NONDISP_AF 	    ( BMAF(Width) | BMAF(Height) | BMAF(PixFmt) )

#define csd 	    	    ((struct class_static_data *)cl->UserData)

/****************************************************************************************/

static OOP_Object *bitmap_new(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
    EnterFunc(bug("BitMap::New()\n"));

    obj  = (OOP_Object *) OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);

    if (NULL != obj)
    {
	struct TagItem      	colmap_tags[] =
	{
	    { aHidd_ColorMap_NumEntries , 16	},
	    { TAG_DONE	    	    	     	}
	};
    	struct HIDDBitMapData 	*data;
	BOOL 	    	    	ok = TRUE;

    	DECLARE_ATTRCHECK(bitmap);
    	IPTR 	    	    	attrs[num_Total_BitMap_Attrs];
	
        data = OOP_INST_DATA(cl, obj);
    
        /* clear all data and set some default values */
        memset(data, 0, sizeof(struct HIDDBitMapData));
	
        data->width         = 320;
        data->height        = 200;
        data->reqdepth	    = 8;
        data->displayable   = FALSE;
	data->pf_registered = FALSE;
	data->modeid 	    = vHidd_ModeID_Invalid;

	if (0 != OOP_ParseAttrs(msg->attrList, attrs, num_Total_BitMap_Attrs,
	    	    	    	&ATTRCHECK(bitmap), HiddBitMapAttrBase))
	{
	    D(bug("!!! ERROR PARSING ATTRS IN BitMap::New() !!!\n"));
	    D(bug("!!! NUMBER OF ATTRS IN IF: %d !!!\n", num_Total_BitMap_Attrs));
	    ok = FALSE;
	}

	if (ok)
	{
	    if (!GOT_BM_ATTR(GfxHidd) || !GOT_BM_ATTR(Displayable))
	    {
    	    	D(bug("!!!! BM CLASS DID NOT GET GFX HIDD !!!\n"));
	    	D(bug("!!!! The reason for this is that the gfxhidd subclass NewBitmap() method\n"));
	    	D(bug("!!!! has not left it to the baseclass to avtually create the object,\n"));
	    	D(bug("!!!! but rather done it itself. This MUST be corrected in the gfxhidd subclass\n"));
		D(bug("!!!! ATTRCHECK: %p !!!!\n", ATTRCHECK(bitmap)));
	    
	    	ok = FALSE;
	    }
	    else
	    {
	    	data->gfxhidd = (OOP_Object *)attrs[AO(GfxHidd)];
	    }
	}
	
	/* Save pointer to friend bitmap */
	if (GOT_BM_ATTR(Friend))
	    data->friend = (OOP_Object *)attrs[AO(Friend)];
	
	if (ok)
	{
	    if ( attrs[AO(Displayable)] )
	    {
		/* We should allways get modeid, but we check anyway */
		if (!GOT_BM_ATTR(ModeID))
		{
		    D(bug("!!! BitMap:New() DID NOT GET MODEID FOR DISPLAYABLE BITMAP !!!\n"));
		    ok = FALSE;
		}
		else
		{
	    	    HIDDT_ModeID    modeid;
	    	    OOP_Object      *sync, *pf;
		
		    modeid = (HIDDT_ModeID)attrs[AO(ModeID)];
	    
	    	    if (!HIDD_Gfx_GetMode(data->gfxhidd, modeid, &sync, &pf))
		    {
	    		D(bug("!!! BitMap::New() RECEIVED INVALID MODEID %p\n", modeid));
	    		ok = FALSE;
	    	    }
		    else
		    {
	    		ULONG width, height;
			
	    		/* Update the bitmap with data from the modeid */
			OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
			OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
			
			data->width = width;
			data->height = height;
			data->displayable = TRUE;

		        /* The PixFmt is allready registered and locked in the PixFmt database */
			data->prot.pixfmt = pf;
		    }
	    	}
	    }
	    else
	    {  /* displayable */
		if (BM_NONDISP_AF != (BM_NONDISP_AF & ATTRCHECK(bitmap)))
		{
		    if (OOP_OCLASS(obj) != CSD(cl)->planarbmclass)
		    {
	    		/* HACK. This is an ugly hack to allow the
		           late initialization of BitMap objects in AROS.
		   
		           The PixelFormat will be set later.
			*/
			
    	    	    	#warning Find a better way to do this.
			
	    	    	/* One could maybe fix this by implementing a separate AmigaPitMap class
	    	    	*/
			
			D(bug("!!! BitMap:New(): NO PIXFMT FOR NONDISPLAYABLE BITMAP !!!\n"));
			ok = FALSE;
		    }
		}
		else
		{
		    data->width	 = attrs[AO(Width)];
		    data->height = attrs[AO(Height)];
		    data->prot.pixfmt = (OOP_Object *)attrs[AO(PixFmt)]; 
		}
		
	    } /* displayable */
	    
	} /* if (ok) */
	
	if (ok)
	{
	    /* initialize the direct method calling */
	    
	    if (GOT_BM_ATTR(ModeID))
	    	data->modeid = attrs[AO(ModeID)];
		
    	#if USE_FAST_PUTPIXEL
	    data->putpixel = (IPTR (*)(OOP_Class *, OOP_Object *, struct pHidd_BitMap_PutPixel *))
			     OOP_GetMethod(obj, CSD(cl)->putpixel_mid);
	    if (NULL == data->putpixel)
		ok = FALSE;
    	#endif
	
    	#if USE_FAST_GETPIXEL
	    data->getpixel = (IPTR (*)(OOP_Class *, OOP_Object *, struct pHidd_BitMap_GetPixel *))
			     OOP_GetMethod(obj, CSD(cl)->getpixel_mid);
	    if (NULL == data->getpixel)
		ok = FALSE;
    	#endif

    	#if USE_FAST_DRAWPIXEL
	    data->drawpixel = (IPTR (*)(OOP_Class *, OOP_Object *, struct pHidd_BitMap_DrawPixel *))
			      OOP_GetMethod(obj, CSD(cl)->drawpixel_mid);
	    if (NULL == data->drawpixel)
		ok = FALSE;
    	#endif
	
	    /* Try to create the colormap */

    	    /* stegerg: Only add a ColorMap for a visible bitmap (screen). This
	                is important because one can create for example a bitmap
			in PIXFMT_LUT8 without friend bitmap and then copy this
			bitmap to a 16 bit screen. During copy the screen bitmap
			CLUT must be used, which would not happen if our PIXFMT_LUT8
			also had a colormap itself because then bltbitmap would use the
			colormap of the PIXFMT_LUT8 bitmap as lookup, which in this
			case would just cause everything to become black in the
			destination (screen) bitmap, because noone ever sets up the
			colormap of the PIXFMT_LUT8 bitmap */
			
    	    if (data->displayable)
	    {
		data->colmap = OOP_NewObject(NULL, CLID_Hidd_ColorMap, colmap_tags);
		if (NULL == data->colmap)
		    ok = FALSE;
	    }
	}
	
	
	if (!ok)
	{
	    ULONG dispose_mid;
	    	
	    dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	
	    OOP_CoerceMethod(cl, obj, (OOP_Msg)&dispose_mid);
	
	    obj = NULL;
	    
    	} /* if(obj) */
	
    } /* if (NULL != obj) */

    ReturnPtr("BitMap::New", OOP_Object *, obj);
}

/****************************************************************************************/

static void bitmap_dispose(OOP_Class *cl, OOP_Object *obj, OOP_Msg *msg)
{
    struct HIDDBitMapData *data = OOP_INST_DATA(cl, obj);

    EnterFunc(bug("BitMap::Dispose()\n"));
    
    if (NULL != data->colmap)
    	OOP_DisposeObject(data->colmap);
    
    D(bug("Calling super\n"));
        
    /* Release the previously registered pixel format */
    if (data->pf_registered)
    	HIDD_Gfx_ReleasePixFmt(data->gfxhidd, data->prot.pixfmt);

    OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);

    ReturnVoid("BitMap::Dispose");
}

/****************************************************************************************/

static VOID bitmap_get(OOP_Class *cl, OOP_Object *obj, struct pRoot_Get *msg)
{
    struct HIDDBitMapData *data = OOP_INST_DATA(cl, obj);
    ULONG   	    	   idx;

    EnterFunc(bug("BitMap::Get() attrID: %i  storage: %p\n", msg->attrID, msg->storage));

    if(IS_BITMAP_ATTR(msg->attrID, idx))
    {
        switch(idx)
        {
            case aoHidd_BitMap_Width:
	    	 *msg->storage = data->width;
		 D(bug("  width: %i\n", data->width));
		 break;
		 
            case aoHidd_BitMap_Height:
	    	*msg->storage = data->height;
		break;
		
            case aoHidd_BitMap_Displayable:
	    	*msg->storage = (IPTR) data->displayable;
		break;
	    
	    case aoHidd_BitMap_PixFmt:
	    	*msg->storage = (IPTR)data->prot.pixfmt;
		break;

	    case aoHidd_BitMap_Friend:
	    	*msg->storage = (IPTR)data->friend;
		break;
		
	    case aoHidd_BitMap_ColorMap:
	    	*msg->storage = (IPTR)data->colmap;
		break;
	    
    	#if 0
            case aoHidd_BitMap_Depth:
	    	if (NULL != data->prot.pixfmt)
		{
	    	    *msg->storage = ((HIDDT_PixelFormat *)data->prot.pixfmt)->depth;
		}
		else
		{
		    *msg->storage = data->reqdepth;
		}
		break;
		
    	#endif
		
	    case aoHidd_BitMap_GfxHidd:
	    	*msg->storage = (IPTR)data->gfxhidd;
		break;
		
	    case aoHidd_BitMap_ModeID:
	    	*msg->storage = data->modeid;
		break;

	    case aoHidd_BitMap_BytesPerRow:
	    {
	    	HIDDT_PixelFormat *pf;
		
		pf = (HIDDT_PixelFormat *)data->prot.pixfmt;
		
		*msg->storage = pf->bytes_per_pixel * data->width;
		break;
	    }
    
            default:
	    	D(bug("UNKNOWN ATTR IN BITMAP BASECLASS: %d\n", idx));
	    	OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);
		break;
        }
    }
    else
    {
	OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);
    }
    
    ReturnVoid("BitMap::Get");
}

/****************************************************************************************/

#define UB(x) ((UBYTE *)x)

/****************************************************************************************/

static BOOL bitmap_setcolors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    /* Copy the colors into the internal buffer */
    struct HIDDBitMapData *data;
    
    data = OOP_INST_DATA(cl, o);

    /* Subclass has initialized HIDDT_Color->pixelVal field and such.
       Just copy it into the colortab.
    */

    if (NULL == data->colmap)
    {
	struct TagItem colmap_tags[] =
	{
    	    { aHidd_ColorMap_NumEntries, 0  },
	    { TAG_DONE	    	    	    }
   	};
	
	colmap_tags[0].ti_Data = msg->firstColor + msg->numColors;
    	data->colmap = OOP_NewObject(NULL, CLID_Hidd_ColorMap, colmap_tags);
    }
    
    if (NULL == data->colmap)
    {
    	return FALSE;
    }

    /* Use the colormap class to set the colors */
    return HIDD_CM_SetColors(data->colmap, msg->colors,
    	    	    	     msg->firstColor, msg->numColors,
			     data->prot.pixfmt);
    
}

/*****************************************************************************************

    BitMap::DrawPixel()

    NAME
        moHidd_BitMap_DrawPixel

    SYNOPSIS
        OOP_DoMethod(obj, WORD x, WORD y);


    FUNCTION
        Changes the pixel at (x,y). The color of the pixel depends on the
        attributes of gc, eg. colors, drawmode, colormask etc.
        This function does not the coordinates.

    INPUTS
        (x,y) - coordinates of the pixel in hidd units

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Gfx_Pixel, GROUP=HIDD_Gfx_SetAttributes

    INTERNALS

    TODO
        - Support for shapeplane.
        - Optimize

*****************************************************************************************/

static ULONG bitmap_drawpixel(OOP_Class *cl, OOP_Object *obj,
    	    	    	      struct pHidd_BitMap_DrawPixel *msg)
{
    HIDDT_Pixel     	    	    src, dest, val;
    HIDDT_DrawMode  	    	    mode;
    HIDDT_Pixel     	    	    writeMask;
    OOP_Object      	    	    *gc;
#if USE_FAST_PUTPIXEL
    struct pHidd_BitMap_PutPixel    p;
#endif

/*    EnterFunc(bug("BitMap::DrawPixel() x: %i, y: %i\n", msg->x, msg->y));
*/
    /*
        Example: Pixels which bits are set to 0 in the colMask must be
                 unchanged

          data->colMask = 001111
          dest          = 101100
                          --

          writeMask = ~data->colMask & dest
                    =   110000       & 101100
                    =   100000

          dest = data->fg && dest = 010100
                                    --

          dest      = dest   & (writeMask | data->ColMask)
                    = 010100 & (100000   | 001111)
                    = 010100 & (101111)
                    = 000100
                      --

          dest      = dest   | writeMask;
                    = 000100   100000
                    = 100100
                      --
    */
    
    gc = msg->gc;

    src       = GC_FG(gc);
    mode      = GC_DRMD(gc);
    
#if OPTIMIZE_DRAWPIXEL_FOR_COPY
    if (vHidd_GC_DrawMode_Copy == mode && GC_COLMASK(gc) == ~0UL)
    {
	val = src;
    }
    else
    {
#endif
        
    dest      = HIDD_BM_GetPixel(obj, msg->x, msg->y);
    writeMask = ~GC_COLMASK(gc) & dest;

    val = 0;

    if(mode & 1) val = ( src &  dest);
    if(mode & 2) val = ( src & ~dest) | val;
    if(mode & 4) val = (~src &  dest) | val;
    if(mode & 8) val = (~src & ~dest) | val;

    val = (val & (writeMask | GC_COLMASK(gc) )) | writeMask;

#if OPTIMIZE_DRAWPIXEL_FOR_COPY
}
#endif

#if USE_FAST_PUTPIXEL
    p.mID	= CSD(cl)->putpixel_mid;
    p.x		= msg->x;
    p.y		= msg->y;
    p.pixel	= val;
    PUTPIXEL(obj, &p);
#else

    HIDD_BM_PutPixel(obj, msg->x, msg->y, val);
#endif

/*    ReturnInt("BitMap::DrawPixel ", ULONG, 1); */ /* in quickmode return always 1 */

    return 1;
}

/*****************************************************************************************

    BitMap::DrawLine()

    NAME
        DrawLine

    SYNOPSIS
        OOP_DoMethod(obj, WORD x1, WORD y1, WORD x2, WORD y2);

   FUNCTION
        Draws a line from (x1,y1) to (x2,y2) in the specified gc.
        The function does not clip the line against the drawing area.

        x1,y1 - start point of the line in hidd units
        x2,y2 - end point of the line in hidd units

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Pixel

    INTERNALS
        Uses midpoint line ("Bresenham") algorithm([FOL90] 3.2.2)

    TODO Support for line pattern
         Optimize remove if t == 1 ...
	 Implement better clipping: Should be no reason to calculate
	 more than the part of the line that is inside the cliprect

    HISTORY

*****************************************************************************************/

static VOID bitmap_drawline(OOP_Class *cl, OOP_Object *obj,
    	    	    	    struct pHidd_BitMap_DrawLine *msg)
{
    WORD    	dx, dy, incrE, incrNE, d, x, y, s1, s2, t, i;
    UWORD   	maskLine = 1 << 15;  /* for line pattern */
    BYTE    	maskCnt  = 16;
    ULONG   	fg;   /* foreground pen   */
    BOOL    	doclip, renderpix;
    
    OOP_Object *gc;


/* bug("BitMap::DrawLine()\n");
*/    EnterFunc(bug("BitMap::DrawLine() x1: %i, y1: %i x2: %i, y2: %i\n", msg->x1, msg->y1, msg->x2, msg->y2));
    
    gc = msg->gc;
    doclip = GC_DOCLIP(gc);
    renderpix = TRUE;
    fg = GC_FG(gc);
    
    if (doclip)
    {
    	/* If line is not inside cliprect, then just return */
	LONG x1, y1, x2, y2;
	
	/* Normalize coords */
	if (msg->x1 > msg->x2)
	{
	    x1 = msg->x2; x2 = msg->x1;
	}
	else
	{
	    x1 = msg->x1; x2 = msg->x2;
	}
	
	if (msg->y1 > msg->y2)
	{
	    y1 = msg->y2; y2 = msg->y1;
	}
	else
	{
	    y1 = msg->y1; y2 = msg->y2;
	}
	
	if (    x1 > GC_CLIPX2(gc)
	     || x2 < GC_CLIPX1(gc)
	     || y1 > GC_CLIPY2(gc)
	     || y2 < GC_CLIPY1(gc) )
	{
	     
	     /* Line is not inside cliprect, so just return */
	     return;
	     
	}
    }

    /* Calculate slope */
    dx = abs(msg->x2 - msg->x1);
    dy = abs(msg->y2 - msg->y1);

    /* which direction? */
    if((msg->x2 - msg->x1) > 0) s1 = 1; else s1 = - 1;
    if((msg->y2 - msg->y1) > 0) s2 = 1; else s2 = - 1;

    /* change axes if dx < dy */
    if(dx < dy)
    {
        d = dx; dx = dy; dy = d; t = 0;
    }
    else
    {
       t = 1;
    }

    d  = 2 * dy - dx;        /* initial value of d */

    incrE  = 2 * dy;         /* Increment use for move to E  */
    incrNE = 2 * (dy - dx);  /* Increment use for move to NE */

    x = msg->x1; y = msg->y1;
    
    if (doclip)
    {
	/* Pixel inside ? */
	if ( POINT_OUTSIDE_CLIP(gc, x, y ))
	    renderpix = FALSE;
	else
	    renderpix = TRUE;
	     
    }
    
    if (renderpix)
    {
	if(GC_LINEPAT(gc) & maskLine)
	{
	     HIDD_BM_DrawPixel(obj, gc, x, y); /* The start pixel */
	}
	else
	{
	    GC_FG(gc) = GC_BG(gc);
	    HIDD_BM_DrawPixel(obj, gc, x, y); /* The start pixel */
            GC_FG(gc) = fg;
	}
    }


    for(i = 0; i < dx; i++)
    {
        maskLine = maskLine >> 1;
        if(--maskCnt == 0)
        {
            maskLine = 1 << 15;  /* for line pattern */
            maskCnt  = 16;
        }

        if(d <= 0)
        {
            if(t == 1)
            {
                x = x + s1;
            }
            else
            {
                y = y + s2;
            }

            d = d + incrE;
        }
        else
        {
            if(t == 1)
            {
                x = x + s1;
                y = y + s2;
            }
            else
            {
                x = x + s1;
                y = y + s2;
            }

            d = d + incrNE;
        }


	if (doclip)
	{
	    /* Pixel inside ? */
	    if ( POINT_OUTSIDE_CLIP(gc, x, y ))
		renderpix = FALSE;
	    else
		renderpix = TRUE;
	     
	}
	
	if (renderpix)
	{
            if(GC_LINEPAT(gc) & maskLine)
	    {
		HIDD_BM_DrawPixel(obj, gc, x, y);
            }
	    else
	    {
		GC_FG(gc) = GC_BG(gc);
		HIDD_BM_DrawPixel(obj, gc, x, y);
		GC_FG(gc) = fg;
	    }
	}
    }

    ReturnVoid("BitMap::DrawLine ");
}

/*****************************************************************************************

    BitMap::DrawRect()

    NAME
        DrawRect

    SYNOPSIS
        OOP_DoMethod(obj,  WORD minX, WORD minY, WORD maxX, WORD maxY);

    FUNCTION

        Draws a hollow rectangle from. minX and minY specifies the upper
        left corner of the rectangle. minY and maxY specifies the lower
        right corner of the rectangle.
        The function does not clip the rectangle against the drawing area.

    INPUTS
        minX, minY - upper left corner of the rectangle in hidd units
        maxX, maxY - lower right corner of the rectangle in hidd units

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Rectangle

    INTERNALS

    TODO

    HISTORY

*****************************************************************************************/

static VOID bitmap_drawrect(OOP_Class *cl, OOP_Object *obj,
    	    	    	    struct pHidd_BitMap_DrawRect *msg)
{
    OOP_Object *gc = msg->gc;
    WORD    	addX, addY;

    EnterFunc(bug("BitMap::DrawRect()"));

    if(msg->minX == msg->maxX) addX = 0; else addX = 1;
    if(msg->minY == msg->maxY) addY = 0; else addY = 1;

    HIDD_BM_DrawLine(obj, gc, msg->minX, msg->minY       , msg->maxX, msg->minY);
    HIDD_BM_DrawLine(obj, gc, msg->maxX, msg->minY + addY, msg->maxX, msg->maxY);
    HIDD_BM_DrawLine(obj, gc, msg->maxX - addX, msg->maxY, msg->minX, msg->maxY);
    HIDD_BM_DrawLine(obj, gc, msg->minX, msg->maxY - addY, msg->minX, msg->minY + addY);

    ReturnVoid("BitMap::DrawRect");
}

/*****************************************************************************************

    BitMap::FillRect()

    NAME
        FillRect

    SYNOPSIS
        OOP_DoMethod(obj,  WORD minX, WORD minY, WORD maxX, WORD maxY);

    FUNCTION

        Draws a solid rectangle. minX and minY specifies the upper
        left corner of the rectangle. minY and maxY specifies the lower
        right corner of the rectangle.
        The function does not clip the rectangle against the drawing area.

    INPUTS
        minX, minY - upper left corner of the rectangle in hidd units
        maxX, maxY - lower right corner of the rectangle in hidd units

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Rectangle

    INTERNALS

    TODO
        Fill with pattern

    HISTORY

*****************************************************************************************/

static VOID bitmap_fillrect(OOP_Class *cl, OOP_Object *obj,
    	    	    	    struct pHidd_BitMap_DrawRect *msg)
{
    OOP_Object *gc = msg->gc;
    WORD    	y = msg->minY;
    
    EnterFunc(bug("BitMap::FillRect()"));

    for(; y <= msg->maxY; y++)
    {
        HIDD_BM_DrawLine(obj, gc, msg->minX, y, msg->maxX, y);
    }

    ReturnVoid("BitMap::FillRect");
}

/*****************************************************************************************

    BitMap::DrawEllipse()
    
    NAME
        DrawEllipse

    SYNOPSIS
        OOP_DoMethod(obj, WORD x, WORD y, UWORD rx, UWORD ry);

    FUNCTION
        Draws a hollow ellipse from the center point (x/y) with the radii
        rx and ry in the specified bitmap.
        The function does not clip the ellipse against the drawing area.

    INPUTS
        x,y   - center point in hidd units
        rx,ry - ry and ry radius in hidd units

    RESULT

    NOTES

    EXAMPLE

    BUGS
        Because of overflow the current code do not work with big
        values of rx and ry.

    SEE ALSO
        GROUP=HIDD_Ellipse

    INTERNALS

    TODO
        Bugfix

    HISTORY

*****************************************************************************************/

#warning Try to opimize clipping here

static VOID bitmap_drawellipse(OOP_Class *cl, OOP_Object *obj,
    	    	    	       struct pHidd_BitMap_DrawEllipse *msg)
{
    OOP_Object *gc = msg->gc;
    WORD    	x = msg->rx, y = 0;     /* ellipse points */

    /* intermediate terms to speed up loop */
    LONG    	t1 = msg->rx * msg->rx, t2 = t1 << 1, t3 = t2 << 1;
    LONG    	t4 = msg->ry * msg->ry, t5 = t4 << 1, t6 = t5 << 1;
    LONG    	t7 = msg->rx * t5, t8 = t7 << 1, t9 = 0L;
    LONG    	d1 = t2 - t7 + (t4 >> 1);    /* error terms */
    LONG    	d2 = (t1 >> 1) - t8 + t5;
    
    BOOL    	doclip = GC_DOCLIP(gc);
    

    EnterFunc(bug("BitMap::DrawEllipse()"));

    while (d2 < 0)                  /* til slope = -1 */
    {
        /* draw 4 points using symmetry */
	
	if  (doclip)
	{
	
	    if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y + y))
		HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y + y);
		
	    if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y - y))
		HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y - y);
		
	    if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y + y))
		HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y + y);
		
	    if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y - y))
		HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y - y);
	     
	}
	else
	{
	    HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y + y);
            HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y - y);
            HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y + y);
            HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y - y);
        }
	
        y++;            /* always move up here */
        t9 = t9 + t3;
        if (d1 < 0)     /* move straight up */
        {
            d1 = d1 + t9 + t2;
            d2 = d2 + t9;
        }
        else            /* move up and left */
        {
            x--;
            t8 = t8 - t6;
            d1 = d1 + t9 + t2 - t8;
            d2 = d2 + t9 + t5 - t8;
        }
    }

    do                              /* rest of top right quadrant */
    {
        /* draw 4 points using symmetry */
    #if 1
	if  (doclip)
	{
	
	    if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y + y))
		HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y + y);
		
	    if (!POINT_OUTSIDE_CLIP(gc, msg->x + x, msg->y - y))
		HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y - y);
		
	    if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y + y))
		HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y + y);
		
	    if (!POINT_OUTSIDE_CLIP(gc, msg->x - x, msg->y - y))
		HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y - y);
	     
	}
	else
	{
	
	    HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y + y);
            HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y - y);
            HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y + y);
            HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y - y);
        }
    #else
	
        HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y + y);
        HIDD_BM_DrawPixel(obj, gc, msg->x + x, msg->y - y);
        HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y + y);
        HIDD_BM_DrawPixel(obj, gc, msg->x - x, msg->y - y);
    #endif
        x--;            /* always move left here */
        t8 = t8 - t6;
        if (d2 < 0)     /* move up and left */
        {
            y++;
            t9 = t9 + t3;
            d2 = d2 + t9 + t5 - t8;
        }
        else            /* move straight left */
        {
            d2 = d2 + t5 - t8;
        }

    } while (x >= 0);


    ReturnVoid("BitMap::DrawEllipse");
}

/*****************************************************************************************

    BitMap::FillEllipse()

    NAME
        FillEllipse

    SYNOPSIS
        OOP_DoMethod(obj, WORD x, WORD y, UWORD rx, UWORD ry);

    FUNCTION
        Draws a solid ellipse from the center point (x/y) with the radii
        rx and ry in the specified bitmap.
        The function does not clip the ellipse against the drawing area.

    INPUTS
        x,y   - center point in hidd units
        rx,ry - ry and ry radius in hidd units

    RESULT

    NOTES

    EXAMPLE

        Because of overflow the current code do not work with big
        values of rx and ry.

    SEE ALSO
        GROUP=HIDD_Ellipse

    INTERNALS

    TODO
        Bugfix

    HISTORY

*****************************************************************************************/

static VOID bitmap_fillellipse(OOP_Class *cl, OOP_Object *obj,
    	    	    	       struct pHidd_BitMap_DrawEllipse *msg)
{
    OOP_Object *gc = msg->gc;
    WORD    	x = msg->rx, y = 0;     /* ellipse points */

    /* intermediate terms to speed up loop */
    LONG    	t1 = msg->rx * msg->rx, t2 = t1 << 1, t3 = t2 << 1;
    LONG    	t4 = msg->ry * msg->ry, t5 = t4 << 1, t6 = t5 << 1;
    LONG    	t7 = msg->rx * t5, t8 = t7 << 1, t9 = 0L;
    LONG    	d1 = t2 - t7 + (t4 >> 1);    /* error terms */
    LONG    	d2 = (t1 >> 1) - t8 + t5;

    EnterFunc(bug("BitMap::FillEllipse()"));

    while (d2 < 0)                  /* til slope = -1 */
    {
        /* draw 4 points using symmetry */
        HIDD_BM_DrawLine(obj, gc, msg->x - x, msg->y + y, msg->x + x, msg->y + y);
        HIDD_BM_DrawLine(obj, gc, msg->x - x, msg->y - y, msg->x + x, msg->y - y);

        y++;            /* always move up here */
        t9 = t9 + t3;
        if (d1 < 0)     /* move straight up */
        {
            d1 = d1 + t9 + t2;
            d2 = d2 + t9;
        }
        else            /* move up and left */
        {
            x--;
            t8 = t8 - t6;
            d1 = d1 + t9 + t2 - t8;
            d2 = d2 + t9 + t5 - t8;
        }
    }

    do                              /* rest of top right quadrant */
    {
        /* draw 4 points using symmetry */
        HIDD_BM_DrawLine(obj, gc, msg->x - x, msg->y + y, msg->x + x, msg->y + y);
        HIDD_BM_DrawLine(obj, gc, msg->x - x, msg->y - y, msg->x + x, msg->y - y);

        x--;            /* always move left here */
        t8 = t8 - t6;
        if (d2 < 0)     /* move up and left */
        {
            y++;
            t9 = t9 + t3;
            d2 = d2 + t9 + t5 - t8;
        }
        else            /* move straight left */
        {
            d2 = d2 + t5 - t8;
        }
	
    } while (x >= 0);

    ReturnVoid("BitMap::FillEllipse");
}

/*****************************************************************************************


    BitMap::DrawPolygon()

    NAME
        DrawPolygon

    SYNOPSIS
        OOP_DoMethod(obj, UWORD n, WORD coords[2*n]);

    FUNCTION
        Draws a hollow polygon from the list of coordinates in coords[].
        The function does not clip the polygon against the drawing area.

    INPUTS
        n      - number of coordinate pairs
        coords - array of n (x, y) coordinates in hidd units

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Polygon

    INTERNALS

    TODO

    HISTORY

*****************************************************************************************/

static VOID bitmap_drawpolygon(OOP_Class *cl, OOP_Object *obj,
    	    	    	       struct pHidd_BitMap_DrawPolygon *msg)
{
    OOP_Object *gc = msg->gc;
    WORD    	i;
    
    EnterFunc(bug("BitMap::DrawPolygon()"));

    for(i = 2; i < (2 * msg->n); i = i + 2)
    {
        HIDD_BM_DrawLine(obj, gc, msg->coords[i - 2], msg->coords[i - 1],
                              msg->coords[i], msg->coords[i + 1]);
    }

    ReturnVoid("BitMap::DrawPolygon");
}

/*****************************************************************************************

    BitMap::FillPolygon()

    NAME
        FillPolygon()

    SYNOPSIS
        OOP_DoMethod(obj, UWORD n, WORD coords[2*n]);

    FUNCTION
        Draws a solid polygon from the list of coordinates in coords[].
        If the last point of the polygon is not equal to the first point
        then the function closes the polygon.

        The function does not clip the polygon against the drawing area.

    INPUTS
        n      - number of coordinate pairs
        coords - array of n (x, y) coordinates in hidd units

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Polygon

    INTERNALS

    TODO

    HISTORY

*****************************************************************************************/

static VOID bitmap_fillpolygon(OOP_Class *cl, OOP_Object *obj, struct pHidd_BitMap_DrawPolygon *msg)
{

    EnterFunc(bug("BitMap::FillPolygon()"));

    D(bug("Sorry, FillPolygon() not implemented yet in bitmap baseclasss\n"));

    ReturnVoid("BitMap::FillPolygon");
}

/*****************************************************************************************

    BitMap::DrawText()

    NAME
        DrawText()

    SYNOPSIS
        OOP_DoMethod(obj, WORD x, WORD y, STRPTR text, UWORD length);

    FUNCTION
        Draws the first length characters of text at (x, y).
        The function does not clip the text against the drawing area.

    INPUTS
        x, y   - Position to start drawing in hidd units. The x
                 coordinate is relativ to the left side of the
                 first character.
                 The y coordinate is relative to the baseline of the font.
        text   - Pointer to a Latin 1 string
        length - Number of characters to draw

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Text

    INTERNALS

    TODO
        - Color fonts
        - Fontstyle

    HISTORY

*****************************************************************************************/

static VOID bitmap_drawtext(OOP_Class *cl, OOP_Object *obj,
    	    	    	    struct pHidd_BitMap_DrawText *msg)
{
    OOP_Object      *gc = msg->gc;
    struct TextFont *font  = GC_FONT(gc);
    UBYTE   	    *charPatternPtr = font->tf_CharData;
    UWORD   	    modulo          = font->tf_Modulo;
    ULONG   	    charLog;
    UBYTE   	    ch;              /* current character to print               */
    WORD    	    fx, fx2, fy, fw; /* position and length of character in the  */
                            /* character bitmap                         */
    WORD    	    xMem = msg->x;   /* position in bitmap                       */
    WORD    	    yMem = msg->y - font->tf_Baseline;
    WORD    	    x, y, i;


    EnterFunc(bug("BitMap::DrawText()"));

    for(i = 0; i < msg->length; i++)
    {
        ch = msg->text[i];
    
        if((ch < font->tf_LoChar) || (ch > font->tf_HiChar))
        {
            ch = font->tf_HiChar - font->tf_LoChar + 1;
        }
        else
        {
            ch = ch - font->tf_LoChar;
        }
    
        if(font->tf_Flags & FPF_PROPORTIONAL)
        {
            xMem = xMem + ((UWORD *) font->tf_CharKern)[ch];
        }
    
        charLog = ((ULONG *) font->tf_CharLoc)[ch];
        fx2 = charLog >> 16;   /* x position of character pattern in character bitmap */
        fw  = (UWORD) charLog; /* width of character pattern in character bitmap */
    
        y = yMem;
    
        for(fy = 0; fy < font->tf_YSize; fy ++)
        {
            x = xMem;
    
            for(fx = fx2; fx < fw + fx2; fx++)
            {
                if(*(charPatternPtr + fx / 8 + fy * modulo) & (128 >> (fx % 8)))
                {
                    HIDD_BM_DrawPixel(obj, msg->gc, x, y);
                }
                x++;
            }
    
            y++;
        }
    
        if(font->tf_Flags & FPF_PROPORTIONAL)
        {
            xMem = xMem + ((UWORD *) font->tf_CharSpace)[ch];
        }
        else
        {
            xMem = xMem + font->tf_XSize;
        }
    }

    ReturnVoid("BitMap::DrawText");
}

/*****************************************************************************************

    BitMap::DrawFillText ()

    NAME
        DrawFillText()

    SYNOPSIS
        OOP_DoMethod(obj, WORD x, WORD y, STRPTR text, UWORD length);

    FUNCTION
        Fills the area of the text with the background color
        and draws the first length characters of text at (x, y).
        The function does not clip the text against the drawing area.

    INPUTS
        x, y   - Position to start drawing in hidd units. The x
                 coordinate is relativ to the left side of the
                 first character.
                 The y coordinate is relative to the baseline of the font.
        text   - Pointer to a Latin 1 string
        length - Number of characters to draw

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Text

    INTERNALS

    TODO

    HISTORY

*****************************************************************************************/

static VOID bitmap_drawfilltext(OOP_Class *cl, OOP_Object *obj, struct pHidd_BitMap_DrawText *msg)
{

    EnterFunc(bug("BitMap::DrawFillText()\n"));

    D(bug("Sorry, DrawFillText() not implemented yet in bitmap baseclass\n"));

    ReturnVoid("BitMap::DrawFillText");
}

/*****************************************************************************************

    BitMap::FillSpan()

    NAME
        FillSpan()

    SYNOPSIS
        OOP_DoMethod(obj, HIDDT_Span span);

    FUNCTION
        Draws a solid from a shape description in the specified bitmap. This
        command is available in quick and normal mode. In normal mode,
        the spans are clipped against the drawing area.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

    HISTORY

*****************************************************************************************/

static VOID bitmap_fillspan(OOP_Class *cl, OOP_Object *obj, struct pHidd_BitMap_DrawText *msg)
{

    EnterFunc(bug("BitMap::FillSpan()\n"));

    D(bug("Sorry, not implemented yet\n"));

    ReturnVoid("BitMap::FillSpan");
}

/*****************************************************************************************

    BitMap::Clear()

    NAME
        Clear()

    SYNOPSIS
        OOP_DoMethod(obj);

    FUNCTION
        Sets all pixels of the drawing area to the background color.
        This command is available in quick and normal mode and behaves
        similar in both modes.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

    HISTORY

*****************************************************************************************/

static VOID bitmap_clear(OOP_Class *cl, OOP_Object *obj, struct pHidd_BitMap_Clear *msg)
{
    WORD  x, y;
    ULONG width, height;

    EnterFunc(bug("BitMap::Clear()\n"));

    OOP_GetAttr(obj, aHidd_BitMap_Width, &width);
    OOP_GetAttr(obj, aHidd_BitMap_Height, &height);

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            HIDD_BM_PutPixel(obj, x, y, 0);
        }
    }

    ReturnVoid("BitMap::Clear");
}

/****************************************************************************************/

static LONG inline getpixfmtbpp(OOP_Class *cl, OOP_Object *o, HIDDT_StdPixFmt stdpf)
{
    OOP_Object *pf;
    struct HIDDBitMapData *data;
    IPTR bpp = -1;
    
    data = OOP_INST_DATA(cl, o);
    
    switch (stdpf)
    {
    	case vHidd_StdPixFmt_Native:
	    OOP_GetAttr(data->prot.pixfmt, aHidd_PixFmt_BytesPerPixel, &bpp);
	    break;
	    
	case vHidd_StdPixFmt_Native32:
	    bpp = sizeof (HIDDT_Pixel);
	    break;
	    
	default:
	    pf = HIDD_Gfx_GetPixFmt(data->gfxhidd, stdpf);
	    
	    if (NULL == pf)
	    {
		D(bug("!!! INVALID PIXFMT IN BitMap::PutImage(): %d !!!\n", stdpf));
	    }
	    else
	    {
		OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bpp);
	    }
	    break;
    }
    
    return bpp;
}

/****************************************************************************************/

static VOID bitmap_getimage(OOP_Class *cl, OOP_Object *o,
    	    	    	    struct pHidd_BitMap_GetImage *msg)
{
    WORD    	    	    x, y;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    LONG    	    	    bpp;
    struct HIDDBitMapData   *data;
    
    data = OOP_INST_DATA(cl, o);
    
    EnterFunc(bug("BitMap::GetImage(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));
    
    
    bpp = getpixfmtbpp(cl, o, msg->pixFmt);
    if (-1 == bpp)
    {
	D(bug("!!! INVALID PIXFMT IN BitMap::PutImage(): %d !!!\n", msg->pixFmt));
	return;
    }
    
        
    switch(msg->pixFmt)
    {
    	case vHidd_StdPixFmt_Native:
	case vHidd_StdPixFmt_Native32:
	    for (y = 0; y < msg->height; y ++)
	    {
    		for (x = 0; x < msg->width; x ++)
    		{
		    register HIDDT_Pixel pix;
		    
		    pix = HIDD_BM_GetPixel(o, x + msg->x , y + msg->y);
		    
		    switch (bpp)
		    {
	    		case 1: *((UBYTE *)pixarray)++ = pix; break;
			case 2: *((UWORD *)pixarray)++ = pix; break;
			case 3: D(bug("GETIMAGE: 3  BYTESPERPIX NOT HANDLED YET\n")); break;
			case 4: *((ULONG *)pixarray)++ = pix; break;
		    }
		    
		}
		
		pixarray += (msg->modulo - msg->width * bpp);
	    }
	    
	    break;
	    
	default:
	    {
		OOP_Object *dstpf;
	    	APTR 	    buf, srcPixels;
		
		dstpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);
		
		buf = srcPixels = AllocVec(msg->width * sizeof(HIDDT_Pixel), MEMF_PUBLIC);
		if (buf)
		{
		    for(y = 0; y < msg->height; y++)
		    {
		    	HIDD_BM_GetImage(o,
					 buf,
					 0,
					 msg->x,
					 msg->y + y,
					 msg->width,
					 1,
					 vHidd_StdPixFmt_Native);

		    	HIDD_BM_ConvertPixels(o,
			    	    	      &srcPixels,
					      (HIDDT_PixelFormat *)data->prot.pixfmt,
					      0,
			    	    	      (APTR *)&pixarray,
					      (HIDDT_PixelFormat *)dstpf,
			    	    	      msg->modulo,
					      msg->width,
					      1,
					      NULL);
					      
		    }
		    FreeVec(buf);
		}
	    }
	    break;
	    
    } /* switch(msg->pixFmt) */
        
    ReturnVoid("BitMap::GetImage");
}

/****************************************************************************************/

static VOID bitmap_putimage(OOP_Class *cl, OOP_Object *o,
    	    	    	    struct pHidd_BitMap_PutImage *msg)
{
    WORD    	    	    x, y;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    ULONG   	    	    old_fg;
    LONG    	    	    bpp;
    struct HIDDBitMapData   *data;   
    OOP_Object	    	    *gc = msg->gc;
    
    data = OOP_INST_DATA(cl, o);
    
    EnterFunc(bug("BitMap::PutImage(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));
    
    
    bpp = getpixfmtbpp(cl, o, msg->pixFmt);
    if (-1 == bpp)
    {
	D(bug("!!! INVALID PIXFMT IN BitMap::PutImage(): %d !!!\n", msg->pixFmt));
	return;
    }
           
    switch(msg->pixFmt)
    {
    	case vHidd_StdPixFmt_Native:
	case vHidd_StdPixFmt_Native32:
	
	    /* Preserve old fg pen */
	    old_fg = GC_FG(gc);
    
	    for (y = 0; y < msg->height; y ++)
	    {
    		for (x = 0; x < msg->width; x ++)
    		{
		    register HIDDT_Pixel pix = 0;

		    switch (bpp)
		    {
	    		case 1:
			    pix = *((UBYTE *)pixarray) & 0x000000FF;
			    pixarray ++;
			    break;
			    
			case 2:
			    pix = *((UWORD *)pixarray) & 0x0000FFFF;
			    pixarray += 2;
			    break;
			    
			case 3:
			    D(bug("PUTIMAGE: 3  BYTESPERPIX NOT HANDLED YET\n"));
			    break;
			    
			case 4:
			    pix = *((ULONG *)pixarray); pixarray += 4;
			    break;

		    }

		    GC_FG(gc) = pix;

		    HIDD_BM_DrawPixel(o, gc, x + msg->x , y + msg->y);
		}
		pixarray += (msg->modulo - msg->width * bpp);
	    }
	    
	    GC_FG(gc) = old_fg;
	    break;
	    
	default:
	    {
		OOP_Object *srcpf;
	    	APTR 	    buf, destPixels;
		
		srcpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);
		
		buf = destPixels = AllocVec(msg->width * sizeof(HIDDT_Pixel), MEMF_PUBLIC);
		if (buf)
		{
		    for(y = 0; y < msg->height; y++)
		    {
		    	HIDD_BM_ConvertPixels(o,
			    	    	      (APTR *)&pixarray,
					      (HIDDT_PixelFormat *)srcpf,
			    	    	      msg->modulo,
					      &destPixels,
					      (HIDDT_PixelFormat *)data->prot.pixfmt,
					      0,
					      msg->width,
					      1,
					      NULL);
					      
		    	HIDD_BM_PutImage(o,
			    	    	 msg->gc,
					 buf,
					 0,
					 msg->x,
					 msg->y + y,
					 msg->width,
					 1,
					 vHidd_StdPixFmt_Native);
		    }
		    FreeVec(buf);
		}
	    }
	    break;
	    
    } /* switch(msg->pixFmt) */
        
    ReturnVoid("BitMap::PutImage");
}

/****************************************************************************************/

static VOID bitmap_putimagelut(OOP_Class *cl, OOP_Object *o,
    	    	    	    struct pHidd_BitMap_PutImageLUT *msg)
{
    WORD    	    	    x, y;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    HIDDT_PixelLUT  	    *pixlut = msg->pixlut;
    HIDDT_Pixel     	    *lut = pixlut ? pixlut->pixels : NULL;
    HIDDT_Pixel     	    *linebuf;
    struct HIDDBitMapData   *data;  
    OOP_Object      	    *gc = msg->gc;
    
    data = OOP_INST_DATA(cl, o);
    
    EnterFunc(bug("BitMap::PutImageLUT(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));
    
    linebuf = AllocVec(msg->width * sizeof(HIDDT_Pixel), MEMF_PUBLIC);
    
    for(y = 0; y < msg->height; y++)
    {
    	if (linebuf)
	{
    	    if (lut)
	    {
    		for(x = 0; x < msg->width; x++)
		{
    	    	    linebuf[x] = lut[pixarray[x]];
		}
	    }
	    else
	    {
    		for(x = 0; x < msg->width; x++)
		{
    	    	    linebuf[x] = pixarray[x];
		}
	    }
	    pixarray += msg->modulo;
	    
	    HIDD_BM_PutImage(o,
	    	    	     msg->gc,
			     (UBYTE *)linebuf,
			     0,
			     msg->x,
			     msg->y + y,
			     msg->width,
			     1,
			     vHidd_StdPixFmt_Native32);
	    
	} /* if (linebuf) */
	else
	{
	    ULONG old_fg;

	    /* Preserve old fg pen */
	    old_fg = GC_FG(gc);

    	    if (lut)
	    {
    		for(x = 0; x < msg->width; x++)
		{
		    GC_FG(gc) = lut[pixarray[x]];		    
    	    	    HIDD_BM_DrawPixel(o, gc, msg->x + x, msg->y + y);
		}
	    }
	    else
	    {
    		for(x = 0; x < msg->width; x++)
		{
		    GC_FG(gc) = pixarray[x];
    	    	    HIDD_BM_DrawPixel(o, gc, msg->x + x, msg->y + y);
		}
	    }
	    GC_FG(gc) = old_fg;
	    
	    pixarray += msg->modulo;
	    
	} /* if (linebuf) else ... */
	
    } /* for(y = 0; y < msg->height; y++) */
    
    if (linebuf) FreeVec(linebuf);
    
    ReturnVoid("BitMap::PutImageLUT");
}

/****************************************************************************************/

static VOID bitmap_getimagelut(OOP_Class *cl, OOP_Object *o,
    	    	    	    struct pHidd_BitMap_GetImageLUT *msg)
{
    WORD    	    	    x, y;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    HIDDT_PixelLUT  	    *pixlut = msg->pixlut;
    HIDDT_Pixel     	    *lut = pixlut ? pixlut->pixels : NULL;
    HIDDT_Pixel     	    *linebuf;
    struct HIDDBitMapData   *data;  
    
    data = OOP_INST_DATA(cl, o);
    
    EnterFunc(bug("BitMap::GetImageLUT(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));
    
    linebuf = AllocVec(msg->width * sizeof(HIDDT_Pixel), MEMF_PUBLIC);
    
    for(y = 0; y < msg->height; y++)
    {
    	if (linebuf)
	{
	    HIDD_BM_GetImage(o,
	    	    	    (UBYTE *)linebuf,
			    0,
			    msg->x,
			    msg->y + y,
			    msg->width,
			    1,
			    vHidd_StdPixFmt_Native32);
    	    if (lut)
	    {
	    	#warning "This is wrong, but HIDD_BM_GetImageLUT on hi/truecolor screens does not really make sense anyway"
    		for(x = 0; x < msg->width; x++)
		{
    	    	    pixarray[x] = (UBYTE)linebuf[x];
		}
	    }
	    else
	    {
    		for(x = 0; x < msg->width; x++)
		{
    	    	    pixarray[x] = (UBYTE)linebuf[x];
		}
	    }
	    pixarray += msg->modulo;
	    	    
	} /* if (linebuf) */
	else
	{
    	    if (lut)
	    {
	    	#warning "This is wrong, but HIDD_BM_GetImageLUT on hi/truecolor screens does not really make sense anyway"
    		for(x = 0; x < msg->width; x++)
		{
		    pixarray[x] = (UBYTE)HIDD_BM_GetPixel(o, msg->x + x, msg->y + y);
		}
	    }
	    else
	    {
    		for(x = 0; x < msg->width; x++)
		{
		    pixarray[x] = (UBYTE)HIDD_BM_GetPixel(o, msg->x + x, msg->y + y);
		}
	    }
	    
	    pixarray += msg->modulo;
	    
	} /* if (linebuf) else ... */
	
    } /* for(y = 0; y < msg->height; y++) */
    
    if (linebuf) FreeVec(linebuf);
    
    ReturnVoid("BitMap::GetImageLUT");
}

/****************************************************************************************/

static VOID bitmap_blitcolexp(OOP_Class *cl, OOP_Object *o,
    	    	    	      struct pHidd_BitMap_BlitColorExpansion *msg)
{
    ULONG   cemd;
    ULONG   fg, bg;
    LONG    x, y;
    
    OOP_Object *gc = msg->gc;
       
    EnterFunc(bug("BitMap::BlitColorExpansion(srcBM=%p, srcX=%d, srcY=%d, destX=%d, destY=%d, width=%d, height=%d)\n",
    		msg->srcBitMap, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
		
    cemd = GC_COLEXP(gc);
    fg	 = GC_FG(gc);
    bg	 = GC_BG(gc);

/* bug("------------- Blit_ColExp: (%d, %d, %d, %d, %d, %d) cemd=%d, fg=%p, bg=%p -------------\n"
    , msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height
    , cemd, fg, bg);
*/    
    for (y = 0; y < msg->height; y ++)
    {
    	for (x = 0; x < msg->width; x ++)
	{
	    ULONG is_set;
	    
	    /* Pixel value is either 0 or 1 for BM of depth 1 */
	    is_set = HIDD_BM_GetPixel(msg->srcBitMap, x + msg->srcX, y + msg->srcY);

/*	    
if (is_set)	
    bug("#");
else
    bug(" "); 
*/	    
	    if (is_set)
	    {
		HIDD_BM_DrawPixel(o, gc, x + msg->destX, y + msg->destY);
	    }
	    else
	    {
		if (cemd & vHidd_GC_ColExp_Opaque)
		{
		    /* Write bixel with BG pen */
		    GC_FG(gc) = bg;
		    HIDD_BM_DrawPixel(o, gc, x + msg->destX, y + msg->destY);
		    /* Reset to FG pen */
		    GC_FG(gc) = fg;
		}
		
	    } /* if () */
	    
	} /* for (each x) */
/*
    bug("\n");
*/
    } /* for ( each y ) */
    
    ReturnVoid("BitMap::BlitColorExpansion");
    
}

/****************************************************************************************/

static ULONG bitmap_bytesperline(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BytesPerLine *msg)
{
     ULONG bpl;
     
     switch (msg->pixFmt)
     {
         case vHidd_StdPixFmt_Native32:
	     bpl = sizeof (HIDDT_Pixel) * msg->width;
	     break;
	     
	 case vHidd_StdPixFmt_Native:
	 {
	     struct HIDDBitMapData *data;
	     
	     data = OOP_INST_DATA(cl, o);
	     
	     bpl = ((HIDDT_PixelFormat *)data->prot.pixfmt)->size * msg->width;
	     break;
	}
	     
	default:
	{
	    OOP_Object      	    *pf;
	    struct HIDDBitMapData   *data;
	    
	    data = OOP_INST_DATA(cl, o);
	    
	    pf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);
	    
	    if (NULL == pf)
	    {
		D(bug("!!! COULD NOT GET STD PIXFMT IN BitMap::BytesPerLine() !!!\n"));
		return 0;
	    }
	    
	    bpl = ((HIDDT_PixelFormat *)pf)->size * msg->width;
	    break;
	}
     }
     
     return bpl;
     
}

/****************************************************************************************/

/*
   This makes it easier to create a subclass of the graphics hidd.
   It only allowd to use this method in the p_RootNew method of a
   bitmap subclass.
*/

/****************************************************************************************/

static VOID bitmap_set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct HIDDBitMapData   *data = OOP_INST_DATA(cl, obj);
    struct TagItem  	    *tag, *tstate;
    ULONG   	    	    idx;

/*    EnterFunc(bug("BitMap::Set()\n"));
*/
    tstate = msg->attrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        if(IS_BITMAP_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_BitMap_Width:
		    data->width = tag->ti_Data;
		    break;
		    
                case aoHidd_BitMap_Height:
		    data->height = tag->ti_Data;
		    break;

    	    #if 0
                case aoHidd_BitMap_ColorTab:
		    data->colorTab = (APTR)tag->ti_Data;
		    break;
    	    #endif

		default:
		    D(bug("!!! TRYING TO SET NONSETTABLE BITMAP ATTR %d !!!\n", idx));
		    break;

            }
        }
    }

    return;
}

/****************************************************************************************/

static OOP_Object *bitmap_setcolormap(OOP_Class *cl, OOP_Object *o,
    	    	    	    	      struct pHidd_BitMap_SetColorMap *msg)
{
    struct HIDDBitMapData   *data;
    OOP_Object	    	    *old;
    
    data = OOP_INST_DATA(cl, o);
    
    old = data->colmap;
    data->colmap = msg->colorMap;
    
    return old;
}

/****************************************************************************************/

static HIDDT_Pixel bitmap_mapcolor(OOP_Class *cl, OOP_Object *o,
    	    	    	    	   struct pHidd_BitMap_MapColor *msg)
{
    HIDDT_PixelFormat *pf = BM_PIXFMT(o);
    
    HIDDT_Pixel red	= msg->color->red;
    HIDDT_Pixel green	= msg->color->green;
    HIDDT_Pixel blue	= msg->color->blue;
        
    /* This code assumes that sizeof (HIDDT_Pixel is a multimple of sizeof(col->#?)
       which should be true for most (all ?) systems. (I have never heard
       of any system with for example 3 byte types.
    */

    if (IS_TRUECOLOR(pf))
    {
    	if (HIDD_PF_SWAPPIXELBYTES(pf))
	{
	    #warning "bitmap_mapcolor assuming that SwapPixelBytes flag only set for 2-byte/16-bit pixel formats"

	    HIDDT_Pixel pixel = MAP_RGB(red, green, blue, pf);
	    
	    SWAPBYTES_WORD(pixel);
	    
	    msg->color->pixval = pixel;
	}
	else
	{
    	    msg->color->pixval = MAP_RGB(red, green, blue, pf);
	}
    }
    else
    {
    	struct HIDDBitMapData 	*data = OOP_INST_DATA(cl, o);
	HIDDT_Color 	    	*ctab;
	
	ctab = ((HIDDT_ColorLUT *)data->colmap)->colors;
	/* Search for the best match in the color table */
#warning Implement this
	
    }

    return msg->color->pixval;
}

/****************************************************************************************/

static VOID bitmap_unmappixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UnmapPixel *msg)
{

    HIDDT_PixelFormat *pf = BM_PIXFMT(o);
    
    if (IS_TRUECOLOR(pf))
    {
    	HIDDT_Pixel pixel = msg->pixel;
	
	if (HIDD_PF_SWAPPIXELBYTES(pf))
	{
	    #warning "bitmap_unmappixel assuming that SwapPixelBytes flag only set for 2-byte/16-bit pixel formats"
	    pixel = SWAPBYTES_WORD(pixel);
	}
	
    	msg->color->red		= RED_COMP	(pixel, pf);
    	msg->color->green	= GREEN_COMP	(pixel, pf);
    	msg->color->blue	= BLUE_COMP	(pixel, pf);
    }
    else
    {
    	struct HIDDBitMapData 	*data = OOP_INST_DATA(cl, o);	
	HIDDT_ColorLUT      	*clut;
	
	clut = (HIDDT_ColorLUT *)data->colmap;
	
	
	
#warning Use CLUT shift and CLUT mask here
	if (msg->pixel < 0 || msg->pixel >= clut->entries)
	    return;
	    
	*msg->color = clut->colors[msg->pixel];
    	
    }
    
    /* Unnecesart, but ... */
    msg->color->pixval	= msg->pixel;
}

/****************************************************************************************/

static BOOL bitmap_obtaindirectaccess(OOP_Class *cl, OOP_Object *o,
    	    	    	    	      struct pHidd_BitMap_ObtainDirectAccess *msg)
{
    /* Default implementation of direct access funcs. Just return FALSE */
    return FALSE;
}

/****************************************************************************************/

static VOID bitmap_releasedirectaccess(OOP_Class *cl, OOP_Object *o,
    	    	    	    	       struct pHidd_BitMap_ReleaseDirectAccess *msg)
{
     D(bug("!!! BitMap BaseClasse ReleaseDirectAccess() called !!!\n"));
     D(bug("!!! This should never happen and is probably due to a buggy implementation in the subclass !!!\n"));
     
     return;
}

/****************************************************************************************/

static VOID bitmap_scalebitmap(OOP_Class * cl, OOP_Object *o,
    	    	    	       struct pHidd_BitMap_BitMapScale * msg)
{
    struct BitScaleArgs     *bsa = msg->bsa;
    UWORD   	    	    ys = bsa-> bsa_SrcY;
    ULONG   	    	    xs = bsa-> bsa_SrcX;
    ULONG   	    	    count = 0;
    ULONG   	    	    dxs = bsa->bsa_SrcWidth;
    ULONG   	    	    dys = bsa->bsa_SrcHeight;
    LONG    	    	    accuyd = - (dys >> 1);
    LONG    	    	    accuxd = - (dxs >> 1);
    ULONG   	    	    y;
    HIDDT_Color     	    col;
    HIDDT_PixelFormat 	    *srcpf, *dstpf;
    OOP_Object      	    *gc = msg->gc;
    OOP_Object      	    *dst, *src;
    HIDDT_Pixel     	    pix;
    UWORD   	    	    *linepattern;

    if (NULL != (linepattern = (UWORD *) AllocMem(2*bsa->bsa_DestHeight, 0)))
    {
        ULONG 	dyd = bsa->bsa_DestHeight;
        ULONG 	dxd = bsa->bsa_DestWidth;
        LONG 	accuys = dyd;
        LONG 	accuxs = dxd;
        UWORD 	DestHeight = bsa->bsa_DestHeight;
        UWORD 	DestWidth  = bsa->bsa_DestWidth + bsa->bsa_DestX;
        /*
         * Fill in the LinePattern array that tells me which original
         * line goes to which destination line.
         */
        while (count < DestHeight)
        {
            accuyd += dys;
            while (accuyd > accuys)
            {
                ys++;
                accuys += dyd;
            }
            linepattern[count] = ys;
    	    //bug("[%d]=%d\n",count, ys);
            count++;
        }
 
        src = msg->src;
        dst = msg->dst;
    
        srcpf = (HIDDT_PixelFormat *)HBM(msg->src)->prot.pixfmt;
        dstpf = (HIDDT_PixelFormat *)HBM(msg->dst)->prot.pixfmt;

        count = bsa->bsa_DestX;
    
        while (count < DestWidth)
        {
            accuxd += dxs;
            while (accuxd > accuxs)
            {
                 xs++;
                 accuxs += dxd;
            }
      
            /*
             * I am copying pixel by pixel only!!!
             */
            for (y = 0; y < bsa->bsa_DestHeight; y++)
            {
    	    	//bug("Reading from source at %d/%d\t",xs,linepattern[y]);
                pix = HIDD_BM_GetPixel(src, xs, linepattern[y]);
                     
                if (srcpf == dstpf)
                    GC_FG(gc) = pix;
                else
                {
                     HIDD_BM_UnmapPixel(src,pix,&col);
                     GC_FG(gc) = HIDD_BM_MapColor(src, &col);
                }
    	    	//bug("Writing to destination at %d/%d\n",count,y);

                HIDD_BM_DrawPixel(dst, gc, count, y);
            } /* for (y =...) */
            count++;
        }/* while */
        FreeMem(linepattern,2*bsa->bsa_DestHeight);
    }
}

/****************************************************************************************/

/* private ! */

/****************************************************************************************/

static BOOL bitmap_setbitmaptags(OOP_Class *cl, OOP_Object *o,
    	    	    	    	 struct pHidd_BitMap_SetBitMapTags *msg)
{
    struct HIDDBitMapData   *data;
    OOP_Object      	    *pf;
    IPTR    	    	    attrs[num_Hidd_BitMap_Attrs];
    DECLARE_ATTRCHECK(bitmap);
    
    data = OOP_INST_DATA(cl, o);
    
    if (0 != OOP_ParseAttrs(msg->bitMapTags
    		, attrs, num_Hidd_BitMap_Attrs
		, &ATTRCHECK(bitmap), HiddBitMapAttrBase))
    {
	D(bug("!!! FAILED PARSING IN BitMap::SetBitMapTags !!!\n"));
	return FALSE;
    }

    if (GOT_BM_ATTR(PixFmtTags))
    {
    	/* Allready a pixfmt registered ? */

	pf = HIDD_Gfx_RegisterPixFmt(data->gfxhidd, (struct TagItem *)attrs[AO(PixFmtTags)]);
	if (NULL == pf)
	    return FALSE;
	    
    	if (data->pf_registered)
	     HIDD_Gfx_ReleasePixFmt(data->gfxhidd, data->prot.pixfmt);

	data->prot.pixfmt = pf;
    }


    if (GOT_BM_ATTR(Width))
    	data->width = attrs[AO(Width)];

    if (GOT_BM_ATTR(Height))	
    	data->height = attrs[AO(Height)];
    
    return TRUE;
}

/****************************************************************************************/

#undef OOPBase
#undef SysBase

#undef csd

#define OOPBase     	    (csd->oopbase)
#define SysBase     	    (csd->sysbase)

#define NUM_ROOT_METHODS    4
#define NUM_BITMAP_METHODS  31

/****************************************************************************************/

OOP_Class *init_bitmapclass(struct class_static_data *csd)
{
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_new     , moRoot_New    },
        {(IPTR (*)())bitmap_dispose , moRoot_Dispose},
        {(IPTR (*)())bitmap_get     , moRoot_Get    },
        {(IPTR (*)())bitmap_set	    , moRoot_Set    },
        {NULL	    	    	    , 0UL   	    }
    };

    struct OOP_MethodDescr bitmap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_setcolors	  	, moHidd_BitMap_SetColors	    },
        {(IPTR (*)())bitmap_drawpixel		, moHidd_BitMap_DrawPixel	    },
        {(IPTR (*)())bitmap_drawline		, moHidd_BitMap_DrawLine	    },
        {(IPTR (*)())bitmap_drawrect		, moHidd_BitMap_DrawRect	    },
        {(IPTR (*)())bitmap_fillrect 		, moHidd_BitMap_FillRect	    },
        {(IPTR (*)())bitmap_drawellipse		, moHidd_BitMap_DrawEllipse	    },
        {(IPTR (*)())bitmap_fillellipse		, moHidd_BitMap_FillEllipse	    },
        {(IPTR (*)())bitmap_drawpolygon		, moHidd_BitMap_DrawPolygon	    },
        {(IPTR (*)())bitmap_fillpolygon		, moHidd_BitMap_FillPolygon	    },
        {(IPTR (*)())bitmap_drawtext		, moHidd_BitMap_DrawText	    },
        {(IPTR (*)())bitmap_drawfilltext	, moHidd_BitMap_FillText	    },
        {(IPTR (*)())bitmap_fillspan		, moHidd_BitMap_FillSpan	    },
        {(IPTR (*)())bitmap_clear		, moHidd_BitMap_Clear		    },
        {(IPTR (*)())bitmap_putimage		, moHidd_BitMap_PutImage	    },
	{(IPTR (*)())bitmap_putimagelut     	, moHidd_BitMap_PutImageLUT 	    },
        {(IPTR (*)())bitmap_getimage		, moHidd_BitMap_GetImage	    },
        {(IPTR (*)())bitmap_getimagelut		, moHidd_BitMap_GetImageLUT	    },
        {(IPTR (*)())bitmap_blitcolexp		, moHidd_BitMap_BlitColorExpansion  },
        {(IPTR (*)())bitmap_bytesperline	, moHidd_BitMap_BytesPerLine	    },
	{(IPTR (*)())bitmap_convertpixels	, moHidd_BitMap_ConvertPixels	    },
	{(IPTR (*)())bitmap_fillmemrect8    	, moHidd_BitMap_FillMemRect8	    },
	{(IPTR (*)())bitmap_fillmemrect16    	, moHidd_BitMap_FillMemRect16	    },
	{(IPTR (*)())bitmap_fillmemrect32    	, moHidd_BitMap_FillMemRect32	    },
	{(IPTR (*)())bitmap_invertmemrect    	, moHidd_BitMap_InvertMemRect	    },
	{(IPTR (*)())bitmap_setcolormap		, moHidd_BitMap_SetColorMap	    },
	{(IPTR (*)())bitmap_mapcolor		, moHidd_BitMap_MapColor	    },
	{(IPTR (*)())bitmap_unmappixel		, moHidd_BitMap_UnmapPixel	    },
	{(IPTR (*)())bitmap_obtaindirectaccess	, moHidd_BitMap_ObtainDirectAccess  },
	{(IPTR (*)())bitmap_releasedirectaccess	, moHidd_BitMap_ReleaseDirectAccess },
	{(IPTR (*)())bitmap_scalebitmap         , moHidd_BitMap_BitMapScale         },

	/* PRIVATE METHODS */	
#warning This is a hack to make the late initialization of planar bitmaps work
	{(IPTR (*)())bitmap_setbitmaptags	, moHidd_BitMap_SetBitMapTags	    },
	
        {NULL	    	    	    	    	, 0UL	    	    	    	    }
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr 	, IID_Root          , NUM_ROOT_METHODS 	    },
        {bitmap_descr	, IID_Hidd_BitMap   , NUM_BITMAP_METHODS    },
        {NULL	    	, NULL	    	    , 0     	    	    }
    };

    OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID	    	, (IPTR) CLID_Root  	    	    	},
        {aMeta_InterfaceDescr	, (IPTR) ifdescr    	    	    	},
        {aMeta_ID   	    	, (IPTR) CLID_Hidd_BitMap   	    	},
        {aMeta_InstSize     	, (IPTR) sizeof(struct HIDDBitMapData)	},
        {TAG_DONE   	    	, 0UL	    	    	    	    	}
    };
    
    OOP_Class *cl = NULL;

    EnterFunc(bug("init_bitmapclass(csd=%p)\n", csd));

    if(MetaAttrBase)
    {
    	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    	if(NULL != cl)
	{
            D(bug("BitMap class ok\n"));
            csd->bitmapclass = cl;
            cl->UserData     = (APTR) csd;
	    OOP_AddClass(cl);
        }
	
    } /* if(MetaAttrBase) */
    
    if (NULL == cl)
	free_bitmapclass(csd);

    ReturnPtr("init_bitmapclass", OOP_Class *,  cl);
}

/****************************************************************************************/

void free_bitmapclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_bitmapclass(csd=%p)\n", csd));

    if(NULL != csd)
    {
	if (NULL != csd->bitmapclass)
	{	
	    OOP_RemoveClass(csd->bitmapclass);
    	    OOP_DisposeObject((OOP_Object *) csd->bitmapclass);
    	    csd->bitmapclass = NULL;
	}
    }

    ReturnVoid("free_bitmapclass");
}

/****************************************************************************************/

#undef OOPBase
#define OOPBase (OOP_OOPBASE(o))

/****************************************************************************************/

#ifndef AROS_CREATE_ROM
#  define STATIC_MID static OOP_MethodID mid
#else
#  define STATIC_MID OOP_MethodID mid = 0
#endif

/****************************************************************************************/

BOOL HIDD_BitMap_SetBitMapTags(OOP_Object *o, struct TagItem *bitMapTags)
{
    STATIC_MID;   
    struct pHidd_BitMap_SetBitMapTags 	p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_SetBitMapTags);

    p.mID = mid;

    p.bitMapTags = bitMapTags;
   
    return (BOOL)OOP_DoMethod(o, (OOP_Msg)&p); 
}

/****************************************************************************************/

/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics bitmap class implementation.
    Lang: english
*/

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <oop/oop.h>
#include <graphics/text.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)

static AttrBase HiddBitMapAttrBase = 0;
static AttrBase HiddGCAttrBase = 0;
static AttrBase HiddPixFmtAttrBase = 0;

/*** BitMap::New() ************************************************************/


/* BitMap baseclass has the following behaviour:

    It can allocate nondisplayable bitmaps, but not
    displayable ones. (Determined by the aHidd_BitMap_Displayable (BOOL) attr).
    If the bitmap is not displayable, then a suitable bitmap
    will be initialized from memory by this baseclass.
    If displayable, then the subclass(es) *MUST* take
    care of initializing a bitmap. This may for example
    be an X11 window for a x11gfx.hidd. Either way, this baseclass
    will initialize and store the width/height etc attrs.
*/    

# warning This may be too slow, could find better solution
/* In the solution used below, subclasses should NOT
   handle aHidd_GC_DrawMode themselves, as they
   will receive a pRoot_Set message for the corresponing
   aHidd_BitMap_xxx attr. Docs should state this.
   
   Alternative 1: Let each subclass get the needed info
   frmom the GC themselves.
   Alt 2.: Like the above, but make the GC a white-box object, and let
         subclasses get the data directly. Possible caveat:
	 GC may theoretically be in different addresspace, or
	 even on differen machine */
	 
static VOID update_from_gc(Class *cl, Object *me, Object *gc)
{

    	/* Update the subclasses */
    struct HIDDBitMapData *data = INST_DATA(cl, me);
    struct TagItem tags[] =
    {
	    { aHidd_BitMap_DrawMode,	0UL },
	    { aHidd_BitMap_Foreground,	0UL },
	    { aHidd_BitMap_Background,	0UL },
	    { aHidd_BitMap_Font, 	0UL },
	    { aHidd_BitMap_ColorMask,   0UL },
	    { aHidd_BitMap_LinePattern, 0UL },
	    { aHidd_BitMap_PlaneMask, 	0UL },
	    { aHidd_BitMap_ColorExpansionMode, 	0UL },
	    { TAG_DONE, 0UL}
    };


    GetAttr(gc, aHidd_GC_DrawMode,	&(tags[0].ti_Data));
    GetAttr(gc, aHidd_GC_Foreground,	&(tags[1].ti_Data));
    GetAttr(gc, aHidd_GC_Background,	&(tags[2].ti_Data));
    GetAttr(gc, aHidd_GC_Font,		&(tags[3].ti_Data));
    GetAttr(gc, aHidd_GC_ColorMask,	&(tags[4].ti_Data));
    GetAttr(gc,	aHidd_GC_LinePattern,	&(tags[5].ti_Data));
    GetAttr(gc, aHidd_GC_PlaneMask, 	&(tags[6].ti_Data));
    GetAttr(gc, aHidd_GC_ColorExpansionMode, 	&(tags[7].ti_Data));

    SetAttrs(me, tags);
    
    data->gc = gc;

    return;
}

static Object *bitmap_new(Class *cl, Object *obj, struct pRoot_New *msg)
{
    struct HIDDBitMapData *data;
    struct TagItem *tag, *tstate;

    EnterFunc(bug("BitMap::New()\n"));

    obj  = (Object *) DoSuperMethod(cl, obj, (Msg) msg);

    if(obj)
    {
        data = INST_DATA(cl, obj);
    
        /* clear all data and set some default values */
        memset(data, 0, sizeof(struct HIDDBitMapData));
        data->width       = 320;
        data->height      = 200;
        data->reqdepth	  = 8;
        data->displayable = FALSE;
        data->format      = vHidd_BitMap_Format_Planar;
        data->fg        = 1;        /* foreground color                        */
        data->bg        = 0;        /* background color                        */
        data->drMode    = vHidd_GC_DrawMode_Copy;    /* drawmode               */
        data->font      = NULL;     /* current fonts                           */
        data->colMask   = ~0;       /* ColorMask prevents some color bits from changing*/
        data->linePat   = ~0;       /* LinePattern                             */
        data->planeMask = NULL;     /* Pointer to a shape bitMap               */

        tstate = msg->attrList;
        while((tag = NextTagItem(&tstate)))
        {
            ULONG idx;

            if(IS_BM_ATTR(tag->ti_Tag, idx))
            {
                switch(idx)
                {
                    case aoHidd_BitMap_Width       : data->width       = tag->ti_Data; break;
                    case aoHidd_BitMap_Height      : data->height      = tag->ti_Data; break;
                    case aoHidd_BitMap_Depth       : data->reqdepth    = tag->ti_Data; break;
                    case aoHidd_BitMap_Displayable : data->displayable = (BOOL) tag->ti_Data; break;
                    case aoHidd_BitMap_Format      : data->format      = tag->ti_Data; break;
		    
		    case aoHidd_BitMap_GC	   : update_from_gc(cl, obj, (Object *)tag->ti_Data); break;
		    
		    case aoHidd_BitMap_Foreground  : data->fg		= tag->ti_Data; break;
		    case aoHidd_BitMap_Background  : data->bg		= tag->ti_Data; break;
		    case aoHidd_BitMap_DrawMode	   : data->drMode	= tag->ti_Data; break;
		    case aoHidd_BitMap_LinePattern : data->linePat	= tag->ti_Data; break;
		    case aoHidd_BitMap_ColorMask   : data->colMask	= tag->ti_Data; break;
		    case aoHidd_BitMap_PlaneMask   : data->planeMask	= (APTR)tag->ti_Data; break;
		    case aoHidd_BitMap_Font	   : data->font		= (APTR)tag->ti_Data; break;
		    case aoHidd_BitMap_Friend	   : data->friend	= (Object *)tag->ti_Data; break;
	    

                    default: D(bug("  unknown attribute %li\n", tag->ti_Data)); break;
                } /* switch tag */
            } /* if (is BM attr) */
        }
    } /* if(obj) */


    ReturnPtr("BitMap::New", Object *, obj);
}


/*** BitMap::Dispose() ********************************************************/

static void bitmap_dispose(Class *cl, Object *obj, Msg *msg)
{
    struct HIDDBitMapData *data = INST_DATA(cl, obj);

    EnterFunc(bug("BitMap::Dispose()\n"));
    
    if (data->coltab)
    {
         D(bug("Has coltab %p\n", data->coltab));
	 FreeVec(data->coltab);
    }
    D(bug("Calling super\n"));

    DoSuperMethod(cl, obj, (Msg) msg);

    ReturnVoid("BitMap::Dispose");
}


/*** BitMap::Get() ************************************************************/

static VOID bitmap_get(Class *cl, Object *obj, struct pRoot_Get *msg)
{
    struct HIDDBitMapData *data = INST_DATA(cl, obj);
    ULONG  idx;

    EnterFunc(bug("BitMap::Get() attrID: %i  storage: %p\n", msg->attrID, msg->storage));

    if(IS_BM_ATTR(msg->attrID, idx))
    {
        switch(idx)
        {
            case aoHidd_BitMap_Width       : *msg->storage = data->width; D(bug("  width: %i\n", data->width)); break;
            case aoHidd_BitMap_Height      : *msg->storage = data->height; break;
            case aoHidd_BitMap_Displayable : *msg->storage = (IPTR) data->displayable; break;
            case aoHidd_BitMap_Format      : *msg->storage = data->format; break;
	    case aoHidd_BitMap_GC	   : *msg->storage = (IPTR) data->gc; break;
	    case aoHidd_BitMap_Foreground  : *msg->storage = data->fg; break;
	    case aoHidd_BitMap_Background  : *msg->storage = data->bg; break;
	    case aoHidd_BitMap_DrawMode	   : *msg->storage = data->drMode; break;
	    case aoHidd_BitMap_LinePattern : *msg->storage = data->linePat; break;
	    case aoHidd_BitMap_ColorMask   : *msg->storage = data->colMask; break;
	    case aoHidd_BitMap_PlaneMask   : *msg->storage = (IPTR) data->planeMask; break;
	    case aoHidd_BitMap_Font	   : *msg->storage = (IPTR) data->font; break;
	    case aoHidd_BitMap_ColorExpansionMode  : *msg->storage = data->colExp; break;
	    case aoHidd_BitMap_Friend	   : *msg->storage = (IPTR)data->friend; break;
	    
            case aoHidd_BitMap_Depth:
	    	if (NULL != data->prot.pixfmt) {
	    	    *msg->storage = ((HIDDT_PixelFormat *)data->prot.pixfmt)->depth;
		} else {
		    *msg->storage = data->reqdepth;
		}
		break;
    
            default: DoSuperMethod(cl, obj, (Msg) msg);
        }
    }

    ReturnVoid("BitMap::Get");
}


/************* BitMap::SetColors() ******************************/
#define UB(x) ((UBYTE *)x)
static BOOL bitmap_setcolors(Class *cl, Object *o, struct pHidd_BitMap_SetColors *msg)
{
    /* Copy the colors into the internal buffer */
    struct HIDDBitMapData *data = INST_DATA(cl, o);
    ULONG copy_size;

    EnterFunc(bug("GfxHIDD.CM::SetColors()\n"));
    
    /* Subclass has initialized HIDDT_Color->pixelVal field and such.
       Just copy it into the colortab.
    */
    if (!data->coltab)
    {
        /* For now palette is hardcoded to 256 colors */
        ULONG coltab_size;
	coltab_size = UB(&data->coltab[255]) - UB(&data->coltab[0]);
	data->coltab = AllocVec(coltab_size, MEMF_ANY);
	if (data->coltab)
	{
	    ULONG i;
	    /* Init to unset, red = 0xFFFF */
	    for (i = 0; i < 256; i ++)
	    {
	    	data->coltab[i].red = 0xFFFF;
	    }
	    
	}
	else
   	    ReturnBool("GfxHIDD.CM::SetColors", TRUE);

    }
    
    copy_size = UB(&data->coltab[msg->numColors]) - UB(&data->coltab[0]);

    /* Copy supplied colors into internal colortab [ CopyMem(src, dest, size) ] */
    CopyMem(msg->colors, &data->coltab[msg->firstColor],  copy_size);
    
    ReturnBool("GfxHIDD.CM::SetColors", TRUE);
}


/*** BitMap::DrawPixel() ***************************************************

    NAME
        moHidd_BitMap_DrawPixel

    SYNOPSIS
        DoMethod(obj, WORD x, WORD y);


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
***************************************************************************/

static ULONG bitmap_drawpixel(Class *cl, Object *obj, struct pHidd_BitMap_DrawPixel *msg)
{
    struct HIDDBitMapData *data = INST_DATA(cl, obj);
    ULONG src, dest, val, mode;
    ULONG writeMask;

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

    src       = data->fg;
    dest      = HIDD_BM_GetPixel(obj, msg->x, msg->y);
    mode      = data->drMode;
    writeMask = ~data->colMask & dest;

    if(mode & 1) val = ( src &  dest);
    if(mode & 2) val = ( src & ~dest) | val;
    if(mode & 4) val = (~src &  dest) | val;
    if(mode & 8) val = (~src & ~dest) | val;

    val = (val & (writeMask | data->colMask)) | writeMask;

    HIDD_BM_PutPixel(obj, msg->x, msg->y, val);


/*    ReturnInt("BitMap::DrawPixel ", ULONG, 1); */ /* in quickmode return always 1 */
    return 1;
}


/*** BitMap::DrawLine() *****************************************************

    NAME
        DrawLine

    SYNOPSIS
        DoMethod(obj, WORD x1, WORD y1, WORD x2, WORD y2);

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

    HISTORY
***************************************************************************/

static VOID bitmap_drawline(Class *cl, Object *obj, struct pHidd_BitMap_DrawLine *msg)
{
    struct HIDDBitMapData *data = INST_DATA(cl, obj);
    WORD  dx, dy, incrE, incrNE, d, x, y, s1, s2, t, i;
    UWORD maskLine = 1 << 15;  /* for line pattern */
    BYTE  maskCnt  = 16;
    ULONG fg       = data->fg;   /* foreground pen   */

    EnterFunc(bug("BitMap::DrawLinel() x1: %i, y1: %i x2: %i, y2: %i\n", msg->x1, msg->y1, msg->x2, msg->y2));

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

    if(data->linePat & maskLine)
    {
        HIDD_BM_DrawPixel(obj, x, y); /* The start pixel */
    }
    else
    {
        data->fg = data->bg;
        HIDD_BM_DrawPixel(obj, x, y); /* The start pixel */
        data->fg = fg;
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

        if(data->linePat & maskLine)
        {
            HIDD_BM_DrawPixel(obj, x, y);
        }
        else
        {
            data->fg = data->bg;
            HIDD_BM_DrawPixel(obj, x, y);
            data->fg = fg;
        }
    }


    ReturnVoid("BitMap::DrawLine ");
}


/*** BitMap::CopyBox() *****************************************************

    NAME
        CopyBox

    SYNOPSIS
        DoMethod(src, WORD srcX, WORD srcY,
                      Object *dest, WORD destX, WORD destY,
                      UWORD sizeX, UWORD sizeY);

   FUNCTION
        Copy a rectangular area from the drawing area src to the drawing
        area stored in dest (which may be src). The source area is not
        changed (except when both rectangles overlap). The mode of the GC
        dest determines how the copy takes place.

        In quick mode, the following restrictions are not checked: It's not
        checked whether the source or destination rectangle is completely
        inside the valid area or whether the areas overlap. If they
        overlap, the results are unpredictable. Also drawing modes are
        ignored. If the two bitmaps in the GCs have a different depth,
        copying might be slow.

        When copying bitmaps between two different HIDDs, the following
        pseudo algorithm is executed: First the destination HIDD is queried
        whether it does understand the format of the source HIDD. If it
        does, then the destination HIDD does the copying. If it doesn't,
        then the source is asked whether it understands the destination
        HIDDs' format. If it does, then the source HIDD will do the
        copying. If it doesn't, then the default CopyArea of the graphics
        HIDD base class will be invoked which copies the bitmaps pixel by
        pixel with BitMap::GetPixel() and BitMap::DrawPixel().

    INPUTS
        src           - source bitmap object
        srcX, srcY    - upper, left corner of the area to copy in the source
        dest          - destination bitmap object
        destX, destY  - upper, left corner in the destination to copy the area
        width, height - width and height of the area in hidd units

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_BltBitMap

    INTERNALS

    TODO

    HISTORY
***************************************************************************/

static VOID bitmap_copybox(Class *cl, Object *obj, struct pHidd_BitMap_CopyBox *msg)
{
    UWORD x, y;
    WORD  srcX = msg->srcX, destX = msg->destX;
    WORD  memSrcX = srcX, memDestX = destX;
    WORD  srcY = msg->srcY, destY = msg->destY;
    ULONG memFG;

    EnterFunc(bug("BitMap::CopyBox()"));

    GetAttr(msg->dest, aHidd_BitMap_Foreground, &memFG);

    for(y = 0; y < msg->height; y++)
    {
        srcX  = memSrcX;
        destX = memDestX;

        for(x = 0; x < msg->width; x++)
        {
            SetAttrsTags(msg->dest, aHidd_BitMap_Foreground,
                               HIDD_BM_GetPixel(obj, srcX++, srcY),
                               TAG_END
                        );
            HIDD_BM_DrawPixel(msg->dest, destX++, destY);
        }

        srcY++; destY++;
    }

    SetAttrsTags(msg->dest, aHidd_BitMap_Foreground, memFG, TAG_END);

    ReturnVoid("BitMap::CopyBox");
}



/*** BitMap::DrawRect() *****************************************************

    NAME
        DrawRect

    SYNOPSIS
        DoMethod(obj,  WORD minX, WORD minY, WORD maxX, WORD maxY);

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
***************************************************************************/

static VOID bitmap_drawrect(Class *cl, Object *obj, struct pHidd_BitMap_DrawRect *msg)
{
    WORD addX, addY;

    EnterFunc(bug("BitMap::DrawRect()"));

    if(msg->minX == msg->maxX) addX = 0; else addX = 1;
    if(msg->minY == msg->maxY) addY = 0; else addY = 1;

    HIDD_BM_DrawLine(obj, msg->minX, msg->minY       , msg->maxX, msg->minY);
    HIDD_BM_DrawLine(obj, msg->maxX, msg->minY + addY, msg->maxX, msg->maxY);
    HIDD_BM_DrawLine(obj, msg->maxX - addX, msg->maxY, msg->minX, msg->maxY);
    HIDD_BM_DrawLine(obj, msg->minX, msg->maxY - addY, msg->minX, msg->minY + addY);

    ReturnVoid("BitMap::DrawRect");
}


/*** BitMap::FillRect() *****************************************************

    NAME
        FillRect

    SYNOPSIS
        DoMethod(obj,  WORD minX, WORD minY, WORD maxX, WORD maxY);

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
***************************************************************************/

static VOID bitmap_fillrect(Class *cl, Object *obj, struct pHidd_BitMap_DrawRect *msg)
{
    WORD y = msg->minY;

    EnterFunc(bug("BitMap::FillRect()"));

    for(; y <= msg->maxY; y++)
    {
        HIDD_BM_DrawLine(obj, msg->minX, y, msg->maxX, y);
    }

    ReturnVoid("BitMap::FillRect");
}


/*** BitMap::DrawEllipse() **************************************************

    NAME
        DrawEllipse

    SYNOPSIS
        DoMethod(obj, WORD x, WORD y, UWORD rx, UWORD ry);

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
***************************************************************************/

static VOID bitmap_drawellipse(Class *cl, Object *obj, struct pHidd_BitMap_DrawEllipse *msg)
{
    WORD   x = msg->rx, y = 0;     /* ellipse points */

    /* intermediate terms to speed up loop */
    LONG t1 = msg->rx * msg->rx, t2 = t1 << 1, t3 = t2 << 1;
    LONG t4 = msg->ry * msg->ry, t5 = t4 << 1, t6 = t5 << 1;
    LONG t7 = msg->rx * t5, t8 = t7 << 1, t9 = 0L;
    LONG d1 = t2 - t7 + (t4 >> 1);    /* error terms */
    LONG d2 = (t1 >> 1) - t8 + t5;

    EnterFunc(bug("BitMap::DrawEllipse()"));

    while (d2 < 0)                  /* til slope = -1 */
    {
        /* draw 4 points using symmetry */
        HIDD_BM_DrawPixel(obj, msg->x + x, msg->y + y);
        HIDD_BM_DrawPixel(obj, msg->x + x, msg->y - y);
        HIDD_BM_DrawPixel(obj, msg->x - x, msg->y + y);
        HIDD_BM_DrawPixel(obj, msg->x - x, msg->y - y);
    
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
        HIDD_BM_DrawPixel(obj, msg->x + x, msg->y + y);
        HIDD_BM_DrawPixel(obj, msg->x + x, msg->y - y);
        HIDD_BM_DrawPixel(obj, msg->x - x, msg->y + y);
        HIDD_BM_DrawPixel(obj, msg->x - x, msg->y - y);
    
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


/*** BitMap::FillEllipse() **************************************************

    NAME
        FillEllipse

    SYNOPSIS
        DoMethod(obj, WORD x, WORD y, UWORD rx, UWORD ry);

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
***************************************************************************/

static VOID bitmap_fillellipse(Class *cl, Object *obj, struct pHidd_BitMap_DrawEllipse *msg)
{
    WORD x = msg->rx, y = 0;     /* ellipse points */

    /* intermediate terms to speed up loop */
    LONG t1 = msg->rx * msg->rx, t2 = t1 << 1, t3 = t2 << 1;
    LONG t4 = msg->ry * msg->ry, t5 = t4 << 1, t6 = t5 << 1;
    LONG t7 = msg->rx * t5, t8 = t7 << 1, t9 = 0L;
    LONG d1 = t2 - t7 + (t4 >> 1);    /* error terms */
    LONG d2 = (t1 >> 1) - t8 + t5;

    EnterFunc(bug("BitMap::FillEllipse()"));

    while (d2 < 0)                  /* til slope = -1 */
    {
        /* draw 4 points using symmetry */
        HIDD_BM_DrawLine(obj, msg->x - x, msg->y + y, msg->x + x, msg->y + y);
        HIDD_BM_DrawLine(obj, msg->x - x, msg->y - y, msg->x + x, msg->y - y);

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
        HIDD_BM_DrawLine(obj, msg->x - x, msg->y + y, msg->x + x, msg->y + y);
        HIDD_BM_DrawLine(obj, msg->x - x, msg->y - y, msg->x + x, msg->y - y);

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



/*** BitMap::DrawPolygon() **************************************************

    NAME
        DrawPolygon

    SYNOPSIS
        DoMethod(obj, UWORD n, WORD coords[2*n]);

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
***************************************************************************/

static VOID bitmap_drawpolygon(Class *cl, Object *obj, struct pHidd_BitMap_DrawPolygon *msg)
{
    WORD i;

    EnterFunc(bug("BitMap::DrawPolygon()"));

    for(i = 2; i < (2 * msg->n); i = i + 2)
    {
        HIDD_BM_DrawLine(obj, msg->coords[i - 2], msg->coords[i - 1],
                              msg->coords[i    ], msg->coords[i + 1]
                        );
    }

    ReturnVoid("BitMap::DrawPolygon");
}



/*** BitMap::FillPolygon() **************************************************

    NAME
        FillPolygon()

    SYNOPSIS
        DoMethod(obj, UWORD n, WORD coords[2*n]);

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
***************************************************************************/

static VOID bitmap_fillpolygon(Class *cl, Object *obj, struct pHidd_BitMap_DrawPolygon *msg)
{

    EnterFunc(bug("BitMap::FillPolygon()"));

    kprintf("Sorry, FillPolygon() not implemented yet in bitmap baseclasss\n");

    ReturnVoid("BitMap::FillPolygon");
}



/*** BitMap::DrawText() *****************************************************

    NAME
        DrawText()

    SYNOPSIS
        DoMethod(obj, WORD x, WORD y, STRPTR text, UWORD length);

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
***************************************************************************/

static VOID bitmap_drawtext(Class *cl, Object *obj, struct pHidd_BitMap_DrawText *msg)
{
    struct HIDDBitMapData *data = INST_DATA(cl, obj);
    struct TextFont *font  = data->font;
    UBYTE  *charPatternPtr = font->tf_CharData;
    UWORD  modulo          = font->tf_Modulo;
    ULONG  charLog;
    UBYTE  ch;              /* current character to print               */
    WORD   fx, fx2, fy, fw; /* position and length of character in the  */
                            /* character bitmap                         */
    WORD   xMem = msg->x;   /* position in bitmap                       */
    WORD   yMem = msg->y - font->tf_Baseline;
    WORD   x, y, i;


    ULONG GetPixel(WORD x, WORD y)
    {
        ULONG offset = x / 8 + y * modulo;
        UBYTE pixel  = 128 >> (x % 8);
    
        if(*(charPatternPtr + offset) & pixel)
        {
            return(1);
        }
        else
        {
            return(0);
        }
    }

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
                if(GetPixel(fx, fy)) HIDD_BM_DrawPixel(obj, x, y);
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


/*** BitMap::DrawFillText () ************************************************

    NAME
        DrawFillText()

    SYNOPSIS
        DoMethod(obj, WORD x, WORD y, STRPTR text, UWORD length);

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
***************************************************************************/

static VOID bitmap_drawfilltext(Class *cl, Object *obj, struct pHidd_BitMap_DrawText *msg)
{

    EnterFunc(bug("BitMap::DrawFillText()\n"));

    kprintf("Sorry, DrawFillText() not implemented yet in bitmap baseclass\n");

    ReturnVoid("BitMap::DrawFillText");
}



/*** BitMap::FillSpan() *****************************************************

    NAME
        FillSpan()

    SYNOPSIS
        DoMethod(obj, HIDDT_Span span);

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
***************************************************************************/

static VOID bitmap_fillspan(Class *cl, Object *obj, struct pHidd_BitMap_DrawText *msg)
{

    EnterFunc(bug("BitMap::FillSpan()\n"));

    D(bug("Sorry, not implemented yet\n"));

    ReturnVoid("BitMap::FillSpan");
}


/*** BitMap::Clear() ********************************************************

    NAME
        Clear()

    SYNOPSIS
        DoMethod(obj);

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
***************************************************************************/

static VOID bitmap_clear(Class *cl, Object *obj, struct pHidd_BitMap_Clear *msg)
{

    WORD  x, y;
    ULONG width, height;

    EnterFunc(bug("BitMap::Clear()\n"));

    GetAttr(obj, aHidd_BitMap_Width, &width);
    GetAttr(obj, aHidd_BitMap_Height, &height);

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            HIDD_BM_PutPixel(obj, x, y, 0);
        }
    }

    ReturnVoid("BitMap::Clear");
}

static VOID bitmap_getimage(Class *cl, Object *o, struct pHidd_BitMap_GetImage *msg)
{
    WORD x, y;
    ULONG *pixarray = (ULONG *)msg->pixels;
    
    for (y = 0; y < msg->height; y ++)
    {
    	for (x = 0; x < msg->width; x ++)
    	{
	    *pixarray ++ = HIDD_BM_GetPixel(o, x + msg->x , y + msg->y);
	}
    }
    return;
}

static VOID bitmap_putimage(Class *cl, Object *o, struct pHidd_BitMap_PutImage *msg)
{
    WORD x, y;
    ULONG *pixarray = (ULONG *)msg->pixels;
    ULONG old_fg;
    struct TagItem fg_tags[] =
    {
	{ aHidd_BitMap_Foreground,	0UL },
	{ TAG_DONE, 0UL}
    };
    
    EnterFunc(bug("BitMap::PutImage(x=%d, y=%d, width=%d, height=%d)\n"
    		, msg->x, msg->y, msg->width, msg->height));
    
    
    /* Preserve old fg pen */
    GetAttr(o, aHidd_BitMap_Foreground, &old_fg);
    
    
    
    for (y = 0; y < msg->height; y ++)
    {
    	for (x = 0; x < msg->width; x ++)
    	{
	    fg_tags[0].ti_Data = *pixarray ++;
	    SetAttrs(o, fg_tags);

	    HIDD_BM_DrawPixel(o, x + msg->x , y + msg->y);
	}
    }
    fg_tags[0].ti_Data = old_fg;
    
    SetAttrs(o, fg_tags);
    
    ReturnVoid("BitMap::PutImage");
}
/*** BitMap::BlitColorExpansion() **********************************************/
static VOID bitmap_blitcolexp(Class *cl, Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{

    ULONG cemd;
    ULONG fg, bg;
    LONG x, y;
    
    struct TagItem fg_tags[] =
    {
	{ aHidd_BitMap_Foreground, 0UL },
	{ TAG_DONE, 0UL }
    };
    
    EnterFunc(bug("BitMap::BlitColorExpansion(srcBM=%p, srcX=%d, srcY=%d, destX=%d, destY=%d, width=%d, height=%d)\n",
    		msg->srcBitMap, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
		
    
    GetAttr(o, aHidd_BitMap_ColorExpansionMode, &cemd);
    GetAttr(o, aHidd_BitMap_Foreground, &fg);
    GetAttr(o, aHidd_BitMap_Background, &bg);
    
    for (y = 0; y < msg->height; y ++)
    {
    	for (x = 0; x < msg->width; x ++)
	{
	    ULONG is_set;
	    
	    /* Pixel value is either 0 or 1 for BM of depth 1 */
	    is_set = HIDD_BM_GetPixel(msg->srcBitMap, x + msg->srcX, y + msg->srcY);
	    
	    if (cemd & vHidd_GC_ColExp_Transparent)
	    {
	    	/* Only use apen if pixel is set */
		if (is_set)
		{
		    HIDD_BM_DrawPixel(o, x + msg->destX, y + msg->destY);
		}
		    
	    }
	    else if (cemd & vHidd_GC_ColExp_Opaque)
	    {
	    	/* Use apen if pixel is et, bpen otherwise */
		if (is_set)
		    HIDD_BM_DrawPixel(o, x + msg->destX, y + msg->destY);
		else
		{
		    /* Write bixel with BG pen */
		    fg_tags[0].ti_Data = bg;
		    SetAttrs(o, fg_tags);
		    HIDD_BM_DrawPixel(o, x + msg->destX, y + msg->destY);
		    /* Reset to FG pen */
		    fg_tags[0].ti_Data = fg;
		    SetAttrs(o, fg_tags);
		    
		}   
		
	    } /* if () */
	    
	} /* for (each x) */
	
    } /* for ( each y ) */
    
    ReturnVoid("BitMap::BlitColorExpansion");
    
}

/*** BitMap::GetPixelFormat() **********************************************/

#if 0
static const HIDDT_PixelFormat std_pixfmts[num_Hidd_StdPixFmt] =
{
    {	/* HIDD_PixFmt_RGB24 */
	24, 24, 3,
	0x00FF0000,
	0x0000FF00,
	0x000000FF,
	0x00000000,
	
	8, 16, 24, 0,
	vHidd_PixFmt_RGB24,
	vHidd_GT_TrueColor
	
    },
    {	/* HIDD_PixFmt_RGB16 */
	16, 16, 2,
	0x0000F800,
	0x000007E0,
	0x0000001F,
	0x00000000,
	
	16, 21, 27, 0,
	vHidd_PixFmt_RGB16,
	vHidd_GT_TrueColor
    },
    {	/* HIDD_PixFmt_ARGB32 */
	32, 32, 4,
	0x00FF0000,
	0x0000FF00,
	0x000000FF,
	0xFF000000,
	
	8, 16, 24, 0,
	vHidd_PixFmt_ARGB32,
	vHidd_GT_TrueColor
    },
    {	/* HIDD_PixFmt_RGBA32 */
	32, 32, 4,
	0xFF000000,
	0x00FF0000,
	0x0000FF00,
	0x000000FF,
	
	0, 8, 16, 24,
	vHidd_PixFmt_RGBA32,
	vHidd_GT_TrueColor
    },
    {	/* HIDD_PixFmt_LUT8 */
	8, 8, 1,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	
	0, 0, 0, 0,
	vHidd_PixFmt_LUT8,
	vHidd_GT_Palette
    }
};

#endif

static Object *bitmap_getpixfmt(Class *cl, Object *o, struct pHidd_BitMap_GetPixelFormat *msg) 
{
    Object *fmt;
    
    kprintf("bitmap_getpixfmt\n");
    
    if (msg->stdPixFmt >= num_Hidd_PixFmt)
    {
    	kprintf("!!! Illegal pixel format passed to BitMap::GetPixelFormat()\n");
	return;
    }
    
    if (msg->stdPixFmt == vHidd_PixFmt_Native || msg->stdPixFmt == vHidd_PixFmt_Native32)
    {
	struct HIDDBitMapData *data;
    
	data = INST_DATA(cl, o);
	fmt  = data->prot.pixfmt;
    }
    else
    {
    	fmt = CSD(cl)->std_pixfmts[msg->stdPixFmt - num_Hidd_PseudoPixFmt];
    }

#if 0    
    if (msg->stdPixFmt == vHidd_PixFmt_Native32)
    {
	msg->pixFmtReturn->bytes_per_pixel = 4;
	msg->pixFmtReturn->stdpixfmt = vHidd_PixFmt_Native32;
    }
#endif

    return fmt;
}


/*** BitMap::BytesPerLine() **********************************************/
static ULONG bitmap_bytesperline(Class *cl, Object *o, struct pHidd_BitMap_BytesPerLine *msg)
{
     Object *pf;
     
     
kprintf("bitmap_bytesperline\n");
     
#warning Optimize this. We can just halfway reimplement bitmap_getpixelformat above
     pf = HIDD_BM_GetPixelFormat(o, msg->pixFmt);
     
     return ((HIDDT_PixelFormat *)pf)->size * msg->width;
     
}
/*** BitMap::Set() *****************************************************/

/*

   This makes it easier to create a subclass of the graphics hidd.
   It only allowd to use this method in the p_RootNew method of a
   bitmap subclass.

*/

static VOID bitmap_set(Class *cl, Object *obj, struct pRoot_Set *msg)
{
    struct HIDDBitMapData *data = INST_DATA(cl, obj);
    struct TagItem *tag, *tstate;
    ULONG  idx;

/*    EnterFunc(bug("BitMap::Set()\n"));
*/
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_BitMap_Width         : data->width         = tag->ti_Data; break;
                case aoHidd_BitMap_Height        : data->height        = tag->ti_Data; break;
                case aoHidd_BitMap_Displayable   : data->displayable   = (BOOL) tag->ti_Data; break;
                case aoHidd_BitMap_Format        : data->format        = tag->ti_Data; break;
                case aoHidd_BitMap_BytesPerRow   : data->bytesPerRow   = tag->ti_Data; break;
                case aoHidd_BitMap_BytesPerPixel : data->bytesPerPixel = tag->ti_Data; break;
                case aoHidd_BitMap_ColorTab      : data->colorTab      = (APTR) tag->ti_Data; break;
		case aoHidd_BitMap_BitMap	 : data->bitMap	       = (Object *)tag->ti_Data;

		case aoHidd_BitMap_GC  		: update_from_gc(cl, obj, (Object *)tag->ti_Data); break;
		case aoHidd_BitMap_Foreground  	: data->fg		= tag->ti_Data; break;
		case aoHidd_BitMap_Background  	: data->bg		= tag->ti_Data; break;
		case aoHidd_BitMap_DrawMode	: data->drMode		= tag->ti_Data; break;
		case aoHidd_BitMap_LinePattern 	: data->linePat		= tag->ti_Data; break;
		case aoHidd_BitMap_ColorMask   	: data->colMask		= tag->ti_Data; break;
		case aoHidd_BitMap_PlaneMask	: (APTR)data->planeMask	= tag->ti_Data; break;
		case aoHidd_BitMap_Font	   	: (APTR)data->font	= tag->ti_Data; break;
		case aoHidd_BitMap_ColorExpansionMode	: data->colExp		= tag->ti_Data; break;

                case aoHidd_BitMap_Depth:
		    if (NULL != data->prot.pixfmt) {
			((HIDDT_PixelFormat *)data->prot.pixfmt)->depth  = tag->ti_Data;
		    } else {
		    	data->reqdepth = tag->ti_Data;
		    }
		    break;

            }
        }
    }

/*    ReturnVoid("BitMap::Set");
*/
    return;
}


/*** init_bitmapclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS   4
#define NUM_BITMAP_METHODS 20

Class *init_bitmapclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_new    , moRoot_New    },
        {(IPTR (*)())bitmap_dispose, moRoot_Dispose},
        {(IPTR (*)())bitmap_get    , moRoot_Get    },
        {(IPTR (*)())bitmap_set	   , moRoot_Set	},
        {NULL, 0UL}
    };

    struct MethodDescr bitmap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_setcolors	  	, moHidd_BitMap_SetColors	},
        {(IPTR (*)())bitmap_drawpixel		, moHidd_BitMap_DrawPixel	},
        {(IPTR (*)())bitmap_drawline		, moHidd_BitMap_DrawLine	},
        {(IPTR (*)())bitmap_copybox		, moHidd_BitMap_CopyBox		},
        {(IPTR (*)())bitmap_drawrect		, moHidd_BitMap_DrawRect	},
        {(IPTR (*)())bitmap_fillrect 		, moHidd_BitMap_FillRect	},
        {(IPTR (*)())bitmap_drawellipse		, moHidd_BitMap_DrawEllipse	},
        {(IPTR (*)())bitmap_fillellipse		, moHidd_BitMap_FillEllipse	},
        {(IPTR (*)())bitmap_drawpolygon		, moHidd_BitMap_DrawPolygon	},
        {(IPTR (*)())bitmap_fillpolygon		, moHidd_BitMap_FillPolygon	},
        {(IPTR (*)())bitmap_drawtext		, moHidd_BitMap_DrawText	},
        {(IPTR (*)())bitmap_drawfilltext	, moHidd_BitMap_FillText	},
        {(IPTR (*)())bitmap_fillspan		, moHidd_BitMap_FillSpan	},
        {(IPTR (*)())bitmap_clear		, moHidd_BitMap_Clear		},
        {(IPTR (*)())bitmap_putimage		, moHidd_BitMap_PutImage		},
        {(IPTR (*)())bitmap_getimage		, moHidd_BitMap_GetImage		},
        {(IPTR (*)())bitmap_blitcolexp		, moHidd_BitMap_BlitColorExpansion	},
        {(IPTR (*)())bitmap_getpixfmt		, moHidd_BitMap_GetPixelFormat	},
        {(IPTR (*)())bitmap_bytesperline	, moHidd_BitMap_BytesPerLine	},
	{(IPTR (*)())bitmap_convertpixels	, moHidd_BitMap_ConvertPixels	},
        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , NUM_ROOT_METHODS},
        {bitmap_descr,  IID_Hidd_BitMap, NUM_BITMAP_METHODS},
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Root},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_ID,             (IPTR) CLID_Hidd_BitMap},
        {aMeta_InstSize,       (IPTR) sizeof(struct HIDDBitMapData)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_bitmapclass(csd=%p)\n", csd));

    if(MetaAttrBase)
    {
        cl = NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("BitMap class ok\n"));
            csd->bitmapclass = cl;
            cl->UserData     = (APTR) csd;
            
            /* Get attrbase for the BitMap interface */
            HiddBitMapAttrBase 	= ObtainAttrBase(IID_Hidd_BitMap);
	    HiddGCAttrBase 	= ObtainAttrBase(IID_Hidd_GC);
	    HiddPixFmtAttrBase 	= ObtainAttrBase(IID_Hidd_PixFmt);
	    
            if(HiddBitMapAttrBase && HiddGCAttrBase && HiddPixFmtAttrBase)
            {
                AddClass(cl);
            }
            else
            {
			
                free_bitmapclass(csd);
                cl = NULL;
            }
        }
    } /* if(MetaAttrBase) */

    ReturnPtr("init_bitmapclass", Class *,  cl);
}


/*** free_bitmapclass *********************************************************/

void free_bitmapclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_bitmapclass(csd=%p)\n", csd));

    if(csd)
    {
        RemoveClass(csd->bitmapclass);
        if(csd->bitmapclass) DisposeObject((Object *) csd->bitmapclass);
        csd->bitmapclass = NULL;
        if(HiddBitMapAttrBase) {
		ReleaseAttrBase(IID_Hidd_BitMap);
		HiddBitMapAttrBase = 0;
	}
	if (HiddGCAttrBase) {
		ReleaseAttrBase(IID_Hidd_GC);
		HiddGCAttrBase = 0;
	}
	
	if (HiddPixFmtAttrBase) {
		ReleaseAttrBase(IID_Hidd_PixFmt);
		HiddPixFmtAttrBase = 0;
	}
    }

    ReturnVoid("free_bitmapclass");
}

/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics gc class implementation.
    Lang: english
*/

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <graphics/text.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#undef  SDEBUG
#define SDEBUG 0
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

static VOID gc_set(Class *cl, Object *obj, struct pRoot_Set *msg);

#define IS_GC_ATTR(attr, idx) ( ( (idx) = (attr) - HiddGCAttrBase) < num_Hidd_GC_Attrs)

static AttrBase HiddGCAttrBase = 0;
static AttrBase HiddBitMapAttrBase = 0;

/*** GC::New() ************************************************************/

static Object *gc_new(Class *cl, Object *obj, struct pRoot_New *msg)
{
    struct HIDDGCData *data;

    struct pRoot_Set set_msg;
    Object *bitMap;

    EnterFunc(bug("GC::New()\n"));

    set_msg.mID = GetMethodID(IID_Root, moRoot_Set);

    /* User MUST supply bitmap */

    bitMap = (APTR) GetTagData(aHidd_GC_BitMap, NULL, msg->attrList);
    if(!bitMap) return NULL;

    obj  = (Object *) DoSuperMethod(cl, obj, (Msg) msg);

    if(obj)
    {
        data = INST_DATA(cl, obj);
    
        /* clear all data and set some default values */

        memset(data, 0, sizeof(struct HIDDGCData));
        data->bitMap    = bitMap;   /* bitmap to which this gc is connected    */
        data->fg        = 1;        /* foreground color                        */
        data->bg        = 0;        /* background color                        */
        data->drMode    = HIDDV_GC_DrawMode_Copy;    /* drawmode               */
        data->font      = NULL;     /* current fonts                           */
        data->colMask   = ~0;       /* ColorMask prevents some color bits from changing*/
        data->linePat   = ~0;       /* LinePattern                             */
        data->planeMask = NULL;     /* Pointer to a shape bitMap               */

        /* Override defaults with user suplied attrs */

        set_msg.attrList = msg->attrList;
/*        gc_set(cl, obj, &set_msg);*/

    } /* if(obj) */

    ReturnPtr("GC::New", Object *, obj);
}


/*** GC::Dispose() ********************************************************/

static void gc_dispose(Class *cl, Object *obj, Msg *msg)
{
    /* struct HIDDGCData *data = INST_DATA(cl, obj); */

    EnterFunc(bug("GC::Dispose()\n"));

    DoSuperMethod(cl, obj, (Msg) msg);

    ReturnVoid("GC::Dispose()");
}


/*** GC::Set() ************************************************************/

static VOID gc_set(Class *cl, Object *obj, struct pRoot_Set *msg)
{
    struct HIDDGCData *data = INST_DATA(cl, obj);
    struct TagItem *tag, *tstate;
    ULONG  idx;

    EnterFunc(bug("GC::Set()\n"));

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_GC_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_GC_Foreground : data->fg        = tag->ti_Data; break;
                case aoHidd_GC_Background : data->bg        = tag->ti_Data; break;
                case aoHidd_GC_DrawMode   : data->drMode    = tag->ti_Data; break;
                case aoHidd_GC_Font       : data->font      = (APTR) tag->ti_Data; break;
                case aoHidd_GC_ColorMask  : data->colMask   = tag->ti_Data; break;
                case aoHidd_GC_LinePattern: data->linePat   = (UWORD) tag->ti_Data; break;
                case aoHidd_GC_PlaneMask  : data->planeMask = (APTR) tag->ti_Data; break;
                case aoHidd_GC_UserData   : data->userData  = (APTR) tag->ti_Data; break;
            }
        }
    }

    ReturnVoid("GC::Set");
}


/*** GC::Get() ************************************************************/

static VOID gc_get(Class *cl, Object *obj, struct pRoot_Get *msg)
{
    struct HIDDGCData *data = INST_DATA(cl, obj);
    ULONG  idx;

    EnterFunc(bug("GC::Get() attrID: %i  storage: %p\n", msg->attrID, msg->storage));

    if(IS_GC_ATTR(msg->attrID, idx))
    {
        switch(idx)
        {
            case aoHidd_GC_BitMap     : *msg->storage = (ULONG) data->bitMap; break;
            case aoHidd_GC_Foreground : *msg->storage = data->fg; break;
            case aoHidd_GC_Background : *msg->storage = data->bg; break;
            case aoHidd_GC_DrawMode   : *msg->storage = data->drMode; break;
            case aoHidd_GC_Font       : *msg->storage = (ULONG) data->font; break;
            case aoHidd_GC_ColorMask  : *msg->storage = data->colMask; break;
            case aoHidd_GC_LinePattern: *msg->storage = data->linePat; break;
            case aoHidd_GC_PlaneMask  : *msg->storage = (ULONG) data->planeMask; break;
            case aoHidd_GC_UserData   : *msg->storage = (ULONG) data->userData; break;
        }
    }

}


/*** GC::WritePixel_Q() ***************************************************

    NAME
        moHidd_GC_WritePixel

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

static ULONG gc_writepixel_q(Class *cl, Object *obj, struct pHidd_GC_WritePixel *msg)
{
    struct HIDDGCData *data = INST_DATA(cl, obj);
    ULONG src, dest, val, mode;
    ULONG writeMask;

    EnterFunc(bug("GC::WritePixel_Q() x: %i, y: %i\n", msg->x, msg->y));

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
    dest      = HIDD_GC_ReadPixel(obj, msg->x, msg->y);
    mode      = data->drMode;
    writeMask = ~data->colMask & dest;

    if(mode & 1) val = ( src &  dest);
    if(mode & 2) val = ( src & ~dest) | val;
    if(mode & 4) val = (~src &  dest) | val;
    if(mode & 8) val = (~src & ~dest) | val;

    val = (val & (writeMask | data->colMask)) | writeMask;

    HIDD_GC_WritePixelDirect(obj, msg->x, msg->y, val);


    ReturnInt("GC::WritePixel_Q ", ULONG, 1); /* in quickmode return always 1 */
}


/*** GC::DrawLine_Q() *****************************************************

    NAME
        DrawLine_Q

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

static VOID gc_drawline_q(Class *cl, Object *obj, struct pHidd_GC_DrawLine *msg)
{
    struct HIDDGCData *data = INST_DATA(cl, obj);
    WORD  dx, dy, incrE, incrNE, d, x, y, s1, s2, t, i;
    UWORD maskLine = 1 << 15;  /* for line pattern */
    BYTE  maskCnt  = 16;
    ULONG fg       = data->fg;   /* foreground pen   */

    EnterFunc(bug("GC::DrawLinel_Q() x1: %i, y1: %i x2: %i, y2: %i\n", msg->x1, msg->y1, msg->x2, msg->y2));

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
        HIDD_GC_WritePixel(obj, x, y); /* The start pixel */
    }
    else
    {
        data->fg = data->bg;
        HIDD_GC_WritePixel(obj, x, y); /* The start pixel */
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
            HIDD_GC_WritePixel(obj, x, y);
        }
        else
        {
            D(bug("BG\n"));
            data->fg = data->bg;
            HIDD_GC_WritePixel(obj, x, y);
            data->fg = fg;
        }
    }


    ReturnVoid("GC::DrawLine_Q ");
}


/*** GC::CopyArea_Q() *****************************************************

    NAME
        CopyArea_Q

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
        pixel with GC::ReadPixel() and GC::WritePixel().

    INPUTS
        src           - source gc object
        srcX, srcY    - upper, left corner of the area to copy in the source
        dest          - destination gc object
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

static VOID gc_copyarea_q(Class *cl, Object *obj, struct pHidd_GC_CopyArea *msg)
{
    UWORD x, y;
    WORD  srcX = msg->srcX, destX = msg->destX;
    WORD  memSrcX = srcX, memDestX = destX;
    WORD  srcY = msg->srcY, destY = msg->destY;
    ULONG memFG;

    EnterFunc(bug("GC::CopyArea_Q()"));

    GetAttr(msg->dest, aHidd_GC_Foreground, &memFG);

    for(y = 0; y < msg->height; y++)
    {
        srcX  = memSrcX;
        destX = memDestX;

        for(x = 0; x < msg->width; x++)
        {
            SetAttrsTags(msg->dest, aHidd_GC_Foreground,
                               HIDD_GC_ReadPixel(obj, srcX++, srcY),
                               TAG_END
                        );
            HIDD_GC_WritePixel(msg->dest, destX++, destY);
        }

        srcY++; destY++;
    }

    SetAttrsTags(msg->dest, aHidd_GC_Foreground, memFG, TAG_END);

    ReturnVoid("GC::CopyArea_Q");
}


#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

/*** GC::DrawRect_Q() *****************************************************

    NAME
        DrawRect_Q

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

static VOID gc_drawrect_q(Class *cl, Object *obj, struct pHidd_GC_DrawRect *msg)
{
    WORD addX, addY;

    EnterFunc(bug("GC::DrawRect_Q()"));

    if(msg->minX == msg->maxX) addX = 0; else addX = 1;
    if(msg->minY == msg->maxY) addY = 0; else addY = 1;

    HIDD_GC_DrawLine(obj, msg->minX, msg->minY       , msg->maxX, msg->minY);
    HIDD_GC_DrawLine(obj, msg->maxX, msg->minY + addY, msg->maxX, msg->maxY);
    HIDD_GC_DrawLine(obj, msg->maxX - addX, msg->maxY, msg->minX, msg->maxY);
    HIDD_GC_DrawLine(obj, msg->minX, msg->maxY - addY, msg->minX, msg->minY + addY);

    ReturnVoid("GC::DrawRect_Q");
}


/*** GC::FillRect_Q() *****************************************************

    NAME
        FillRect_Q

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

static VOID gc_fillrect_q(Class *cl, Object *obj, struct pHidd_GC_DrawRect *msg)
{
    WORD y = msg->minY;

    EnterFunc(bug("GC::FillRect_Q()"));

    for(; y <= msg->maxY; y++)
    {
        HIDD_GC_DrawLine(obj, msg->minX, y, msg->maxX, y);
    }

    ReturnVoid("GC::FillRect_Q");
}


/*** GC::DrawEllipse_Q() **************************************************

    NAME
        DrawEllipse_Q

    SYNOPSIS
        DoMethod(obj, WORD x, WORD y, UWORD rx, UWORD ry);

    FUNCTION
        Draws a hollow ellipse from the center point (x/y) with the radii
        rx and ry in the specified gc.
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

static VOID gc_drawellipse_q(Class *cl, Object *obj, struct pHidd_GC_DrawEllipse *msg)
{
    WORD   x = msg->rx, y = 0;     /* ellipse points */

    /* intermediate terms to speed up loop */
    LONG t1 = msg->rx * msg->rx, t2 = t1 << 1, t3 = t2 << 1;
    LONG t4 = msg->ry * msg->ry, t5 = t4 << 1, t6 = t5 << 1;
    LONG t7 = msg->rx * t5, t8 = t7 << 1, t9 = 0L;
    LONG d1 = t2 - t7 + (t4 >> 1);    /* error terms */
    LONG d2 = (t1 >> 1) - t8 + t5;

    EnterFunc(bug("GC::DrawEllipse_Q()"));

    while (d2 < 0)                  /* til slope = -1 */
    {
        /* draw 4 points using symmetry */
        HIDD_GC_WritePixel(obj, msg->x + x, msg->y + y);
        HIDD_GC_WritePixel(obj, msg->x + x, msg->y - y);
        HIDD_GC_WritePixel(obj, msg->x - x, msg->y + y);
        HIDD_GC_WritePixel(obj, msg->x - x, msg->y - y);
    
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
        HIDD_GC_WritePixel(obj, msg->x + x, msg->y + y);
        HIDD_GC_WritePixel(obj, msg->x + x, msg->y - y);
        HIDD_GC_WritePixel(obj, msg->x - x, msg->y + y);
        HIDD_GC_WritePixel(obj, msg->x - x, msg->y - y);
    
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


    ReturnVoid("GC::DrawEllipse_Q");
}


/*** GC::FillEllipse_Q() **************************************************

    NAME
        FillEllipse_Q

    SYNOPSIS
        DoMethod(obj, WORD x, WORD y, UWORD rx, UWORD ry);

    FUNCTION
        Draws a solid ellipse from the center point (x/y) with the radii
        rx and ry in the specified gc.
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

static VOID gc_fillellipse_q(Class *cl, Object *obj, struct pHidd_GC_DrawEllipse *msg)
{
    WORD x = msg->rx, y = 0;     /* ellipse points */

    /* intermediate terms to speed up loop */
    LONG t1 = msg->rx * msg->rx, t2 = t1 << 1, t3 = t2 << 1;
    LONG t4 = msg->ry * msg->ry, t5 = t4 << 1, t6 = t5 << 1;
    LONG t7 = msg->rx * t5, t8 = t7 << 1, t9 = 0L;
    LONG d1 = t2 - t7 + (t4 >> 1);    /* error terms */
    LONG d2 = (t1 >> 1) - t8 + t5;

    EnterFunc(bug("GC::FillEllipse_Q()"));

    while (d2 < 0)                  /* til slope = -1 */
    {
        /* draw 4 points using symmetry */
        HIDD_GC_DrawLine(obj, msg->x - x, msg->y + y, msg->x + x, msg->y + y);
        HIDD_GC_DrawLine(obj, msg->x - x, msg->y - y, msg->x + x, msg->y - y);

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
        HIDD_GC_DrawLine(obj, msg->x - x, msg->y + y, msg->x + x, msg->y + y);
        HIDD_GC_DrawLine(obj, msg->x - x, msg->y - y, msg->x + x, msg->y - y);

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


    ReturnVoid("GC::FillEllipse_Q");
}


#undef  SDEBUG
#define SDEBUG 0
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*** GC::DrawPolygon_Q() **************************************************

    NAME
        DrawPolygon_Q

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

static VOID gc_drawpolygon_q(Class *cl, Object *obj, struct pHidd_GC_DrawPolygon *msg)
{
    WORD i;

    EnterFunc(bug("GC::DrawPolygon_Q()"));

    for(i = 2; i < (2 * msg->n); i = i + 2)
    {
        HIDD_GC_DrawLine(obj, msg->coords[i - 2], msg->coords[i - 1],
                              msg->coords[i    ], msg->coords[i + 1]
                        );
    }

    ReturnVoid("GC::DrawPolygon_Q");
}


#undef  SDEBUG
#define SDEBUG 0
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

/*** GC::FillPolygon_Q() **************************************************

    NAME
        FillPolygon_Q()

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

static VOID gc_fillpolygon_q(Class *cl, Object *obj, struct pHidd_GC_DrawPolygon *msg)
{
    struct HIDDGCData *data = INST_DATA(cl, obj);

    EnterFunc(bug("GC::FillPolygon_Q()"));

    D(bug("Sorry, not implemented yet\n"));

    ReturnVoid("GC::FillPolygon_Q");
}


#undef  SDEBUG
#define SDEBUG 0
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

/*** GC::DrawText_Q() *****************************************************

    NAME
        DrawText_Q()

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

static VOID gc_drawtext_q(Class *cl, Object *obj, struct pHidd_GC_DrawText *msg)
{
    struct HIDDGCData *data = INST_DATA(cl, obj);
    struct TextFont *font  = data->font;
    UBYTE  *charPatternPtr = font->tf_CharData;
    UWORD  modulo          = font->tf_Modulo;
    ULONG  charLog;
    UBYTE  ch;              /* current character to print               */
    WORD   fx, fx2, fy, fw; /* position and length of character in the  */
                            /* character bitmap                         */
    WORD   xMem = msg->x;   /* position in gc                           */
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

    EnterFunc(bug("GC::DrawText_Q()"));

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
                if(GetPixel(fx, fy)) HIDD_GC_WritePixel(obj, x, y);
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

    ReturnVoid("GC::DrawText_Q");
}


/*** GC::DrawFillText_Q () ************************************************

    NAME
        DrawFillText_Q()

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

static VOID gc_drawfilltext_q(Class *cl, Object *obj, struct pHidd_GC_DrawText *msg)
{
    struct HIDDGCData *data = INST_DATA(cl, obj);

    EnterFunc(bug("GC::DrawFillText_Q()\n"));

    D(bug("Sorry, not implemented yet\n"));

    ReturnVoid("GC::DrawFillText_Q");
}


#undef  SDEBUG
#define SDEBUG 1
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

/*** GC::FillSpan_Q() *****************************************************

    NAME
        FillSpan_Q()

    SYNOPSIS
        DoMethod(obj, HIDDT_Span span);

    FUNCTION
        Draws a solid from a shape description in the specified gc. This
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

static VOID gc_fillspan_q(Class *cl, Object *obj, struct pHidd_GC_DrawText *msg)
{
    struct HIDDGCData *data = INST_DATA(cl, obj);

    EnterFunc(bug("GC::FillSpan_Q()\n"));

    D(bug("Sorry, not implemented yet\n"));

    ReturnVoid("GC::FillSpan_Q");
}


/*** GC::Clear_Q() ********************************************************

    NAME
        Clear_Q()

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

static VOID gc_clear_q(Class *cl, Object *obj, struct pHidd_GC_Clear *msg)
{
    struct HIDDGCData *data = INST_DATA(cl, obj);

    WORD  x, y;
    ULONG width, height;

    EnterFunc(bug("GC::Clear_Q()\n"));

    GetAttr(data->bitMap, aHidd_BitMap_Width, &width);
    GetAttr(data->bitMap, aHidd_BitMap_Height, &height);

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            HIDD_GC_WritePixelDirect(obj, x, y, 0);
        }
    }

    ReturnVoid("GC::Clear_Q");
}

static VOID gc_readpixelarray(Class *cl, Object *o, struct pHidd_GC_ReadPixelArray *msg)
{
    WORD x, y;
    ULONG *pixarray = msg->pixelArray;
    
    for (y = 0; y < msg->height; y ++)
    {
    	for (x = 0; x < msg->width; x ++)
    	{
	    *pixarray ++ = HIDD_GC_ReadPixel(o, x + msg->x , y + msg->y);
	}
    }
    return;
}

static VOID gc_writepixelarray(Class *cl, Object *o, struct pHidd_GC_WritePixelArray *msg)
{
    WORD x, y;
    ULONG *pixarray = msg->pixelArray;
    ULONG old_fg;
    struct HIDDGCData *data = INST_DATA(cl, o);
    
    /* Preserver old fg pen */
    old_fg = data->fg;
    
    
    
    for (y = 0; y < msg->height; y ++)
    {
    	for (x = 0; x < msg->width; x ++)
    	{
	    data->fg = *pixarray ++;
	    HIDD_GC_WritePixel(o, x + msg->x , y + msg->y);
	}
    }
    data->fg = old_fg;
    return;
}

/*** init_gcclass *************************************************************/

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)
#define UtilityBase (csd->utilitybase)

#define NUM_ROOT_METHODS   4
#define NUM_GC_METHODS    17

Class *init_gcclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())gc_new         , moRoot_New    },
        {(IPTR (*)())gc_dispose     , moRoot_Dispose},
        {(IPTR (*)())gc_set         , moRoot_Set    },
        {(IPTR (*)())gc_get         , moRoot_Get    },
        {NULL, 0UL}
    };

    struct MethodDescr gc_descr[NUM_GC_METHODS + 1] =
    {
        {(IPTR (*)())gc_writepixeldirect_q, moHidd_GC_WritePixelDirect},
        {(IPTR (*)())gc_writepixel_q      , moHidd_GC_WritePixel},
        {(IPTR (*)())gc_readpixel_q       , moHidd_GC_ReadPixel},
        {(IPTR (*)())gc_drawline_q        , moHidd_GC_DrawLine},
        {(IPTR (*)())gc_copyarea_q        , moHidd_GC_CopyArea},
        {(IPTR (*)())gc_drawrect_q        , moHidd_GC_DrawRect},
        {(IPTR (*)())gc_fillrect_q        , moHidd_GC_FillRect},
        {(IPTR (*)())gc_drawellipse_q     , moHidd_GC_DrawEllipse},
        {(IPTR (*)())gc_fillellipse_q     , moHidd_GC_FillEllipse},
        {(IPTR (*)())gc_drawpolygon_q     , moHidd_GC_DrawPolygon},
        {(IPTR (*)())gc_fillpolygon_q     , moHidd_GC_FillPolygon},
        {(IPTR (*)())gc_drawtext_q        , moHidd_GC_DrawText},
        {(IPTR (*)())gc_drawfilltext_q    , moHidd_GC_FillText},
        {(IPTR (*)())gc_fillspan_q        , moHidd_GC_FillSpan},
        {(IPTR (*)())gc_clear_q           , moHidd_GC_Clear},
        {(IPTR (*)())gc_readpixelarray    , moHidd_GC_ReadPixelArray},
        {(IPTR (*)())gc_writepixelarray   , moHidd_GC_WritePixelArray},
        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root        , NUM_ROOT_METHODS},
        {gc_descr  ,    IID_Hidd_GC     , NUM_GC_METHODS  },
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Root},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_ID,             (IPTR) CLID_Hidd_GCQuick},
        {aMeta_InstSize,       (IPTR) sizeof(struct HIDDGCData)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_gcclass(csd=%p)\n", csd));

    if(MetaAttrBase)
    {
        HiddBitMapAttrBase = GetAttrBase(IID_Hidd_BitMap);
        if(HiddBitMapAttrBase)
        {
            cl = NewObject(NULL, CLID_HiddMeta, tags);
            if(cl)
            {
                D(bug("GC class ok\n"));
                csd->gcclass = cl;
                cl->UserData = (APTR) csd;
                
                /* Get attrbase for the GC interface */
                HiddGCAttrBase = ObtainAttrBase(IID_Hidd_GC);
                if(HiddGCAttrBase)
                {
                    AddClass(cl);
                }
                else
                {
                    free_gcclass(csd);
                    cl = NULL;
                }
            }
        } /* if(HiddBitMapAttrBase) */
    } /* if(MetaAttrBase) */

    ReturnPtr("init_gcclass", Class *,  cl);
}

/*** free_gcclass *************************************************************/

void free_gcclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_gcclass(csd=%p)\n", csd));

    if(csd)
    {
        D(bug("1\n"));
        RemoveClass(csd->gcclass);
        D(bug("2\n"));

        if(csd->gcclass) DisposeObject((Object *) csd->gcclass);
        D(bug("3\n"));

        csd->gcclass = NULL;
        D(bug("4\n"));

        if(HiddGCAttrBase)     ReleaseAttrBase(IID_Hidd_GC);
        if(HiddBitMapAttrBase) ReleaseAttrBase(IID_Hidd_BitMap);
        D(bug("5\n"));
    }

    ReturnVoid("free_gcclass");
}


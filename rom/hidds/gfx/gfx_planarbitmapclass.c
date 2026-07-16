/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Gfx Hidd planar bitmap class implementation.
*/

/****************************************************************************************/

#include "gfx_debug.h"

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <graphics/gfx.h>
#include <oop/oop.h>

#include <hidd/gfx.h>

#include <string.h>

#include "gfx_intern.h"

/*****************************************************************************************

    NAME
        --background_planarbm--

    LOCATION
        hidd.gfx.bitmap.planarbm

    NOTES
        This is a class representing a planar Amiga(tm) bitmap in AROS graphics subsystem.

        When you create an object of this class, an associated planar bitmap will be created.
        However, it's possible to use this class with pre-existing bitmaps, making them
        available to the Gfx Hidd subsystem.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_PlanarBM_AllocPlanes

    SYNOPSIS
        [I..], BOOL

    LOCATION
        hidd.gfx.bitmap.planarbm

    FUNCTION
        Set this attribute to FALSE if you want to create an empty bitmap object containing
        no bitmap data. Useful if you want to create an empty object to be associated with
        existing bitmap later.

    NOTES
        This attribute is obsolete. It's equal to supplying aoHidd_PlanarBM_BitMap attribute
        with a NULL value.

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_PlanarBM_BitMap

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_PlanarBM_BitMap

    SYNOPSIS
        [ISG], struct BitMap *

    LOCATION
        hidd.gfx.bitmap.planarbm

    FUNCTION
        Allows to specify or retrieve a raw planar bitmap structure associated with the object.
        Useful for direct access to the bitmap within subclasses, as well as for associating
        an object with already existing BitMap structure.

        It is valid to pass this attribute with a NULL value. In this case the object becomes
        empty and contains no actual bitmap.

    NOTES
        If the object was created with own bitmap data (with no aoHidd_PlanarBM_BitMap specified
        during creation), this data will be deallocated when you set this attribute.

        It's up to you to deallocate own bitmaps, set using this attribute. Even if the object
        is disposed, it won't deallocate user-supplied bitmap.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

OOP_Object *PBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    IPTR height, bytesperrow;
    UBYTE depth;
    IPTR displayable = FALSE;
    BOOL ok = FALSE;
    struct planarbm_data *data;
    struct TagItem *tag;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);
    if (NULL == o)
        return NULL;

    data = OOP_INST_DATA(cl, o);

    /* Check if we want to use existing bitmap */

    tag = FindTagItem(aHidd_PlanarBM_BitMap, msg->attrList);
    if (tag)
    {
        /* It's not our own bitmap */
        data->planes_alloced = FALSE;
        /* Remember the bitmap. It can be NULL here. */
        data->bitmap = (struct BitMap *)tag->ti_Data;

        /* That's all, we are attached to an existing BitMap */
        return o;
    }
    else
    {
        /* Check obsolete attribute now */
        data->planes_alloced = GetTagData(aHidd_PlanarBM_AllocPlanes, TRUE, msg->attrList);

        if (!data->planes_alloced)
            return o; /* Late initialization */
    }

    /* By default we create 1-plane bitmap */
    depth = GetTagData(aHidd_BitMap_Depth, 1, msg->attrList);

    /* Not late initialization. Get some info on the bitmap */
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    OOP_GetAttr(o, aHidd_BitMap_BytesPerRow, &bytesperrow);
    OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);

    data->bitmap = AllocMem(sizeof(struct BitMap), MEMF_CLEAR);
    if (data->bitmap)
    {
        UBYTE i;

        ok = TRUE;

        /* We cache some info */
        data->bitmap->Rows        = height;
        data->bitmap->BytesPerRow = bytesperrow;
        data->bitmap->Depth       = depth;
        data->bitmap->Flags       = BMF_STANDARD|BMF_MINPLANES; /* CHECKME */
        if (displayable)
            data->bitmap->Flags |= BMF_DISPLAYABLE;

        /*
         * Allocate memory for all the planes. Use chip memory.
         *
         * Displayable bitmaps (screens, framebuffers) are always fully painted
         * before/at display, and graphics.library's AllocBitMap() blit-clears
         * them itself when the caller passes BMF_CLEAR, so zeroing the planes
         * here with a CPU memset is redundant work on the boot path. Only clear
         * offscreen bitmaps, whose fresh contents a caller may read before any
         * draw. (MEMF_CLEAR is expensive on the 68000; skipping the full-screen
         * clear saves a large memset at boot.)
         */
        ULONG planeflags = MEMF_CHIP | (displayable ? 0 : MEMF_CLEAR);
        for (i = 0; i < depth; i++)
        {
            data->bitmap->Planes[i] = AllocMem(height * bytesperrow, planeflags);

            if (NULL == data->bitmap->Planes[i])
            {
                ok = FALSE;
                break;
            }
        }
    }

    if (!ok)
    {
        OOP_MethodID dispose_mid;

        dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);

        o = NULL;
    }
        
    return o;
}

/****************************************************************************************/

static void PBM_FreeBitMap(struct planarbm_data *data)
{
    if (data->planes_alloced)
    {
        if (NULL != data->bitmap)
        {
            UBYTE i;

            for (i = 0; i < data->bitmap->Depth; i++)
            {
                if (data->bitmap->Planes[i])
                {
                    FreeMem(data->bitmap->Planes[i], data->bitmap->Rows * data->bitmap->BytesPerRow);
                }
            }
            FreeMem(data->bitmap, sizeof(struct BitMap));
        }
    }
}

VOID PBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct planarbm_data *data = OOP_INST_DATA(cl, o);

    PBM_FreeBitMap(data);
    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

VOID PBM__Root__Get(OOP_Class *cl, OOP_Object *obj, struct pRoot_Get *msg)
{
    struct planarbm_data *data = OOP_INST_DATA(cl, obj);

    if (msg->attrID == aHidd_BitMap_Depth)
    {
        /* Planar bitmaps may have a variable depth. */
        *msg->storage = data->bitmap ? data->bitmap->Depth : 0;
        return;
    }
    else if (msg->attrID == aHidd_PlanarBM_BitMap)
    {
        *msg->storage = (IPTR)data->bitmap;
        return;
    }

    OOP_DoSuperMethod(cl, obj, &msg->mID);
}

/****************************************************************************************/

static BOOL PBM_SetBitMap(OOP_Class *cl, OOP_Object *o, struct BitMap *bm)
{
    struct TagItem pftags[] =
    {
        { aHidd_PixFmt_Depth        , 0UL                       },      /* 0 */
        { aHidd_PixFmt_BitsPerPixel , 0UL                       },      /* 1 */
        { aHidd_PixFmt_BytesPerPixel, 1UL                       },      /* 2 */
        { aHidd_PixFmt_ColorModel   , vHidd_ColorModel_Palette  },      /* 3 */
        { aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Planar   },      /* 4 */
        { aHidd_PixFmt_CLUTShift    , 0UL                       },      /* 5 */
        { aHidd_PixFmt_CLUTMask     , 0x000000FF                },      /* 6 */
        { aHidd_PixFmt_RedMask      , 0x00FF0000                },      /* 7 */
        { aHidd_PixFmt_GreenMask    , 0x0000FF00                },      /* 8 */
        { aHidd_PixFmt_BlueMask     , 0x000000FF                },      /* 9 */
        { aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_Plane     },
        { TAG_DONE                  , 0UL                       }
    };
    struct TagItem      bmtags[] =
    {
        { aHidd_BitMap_Width        , 0 }, /* 0 */
        { aHidd_BitMap_Height       , 0 }, /* 1 */
        { aHidd_BitMap_BytesPerRow  , 0 }, /* 2 */
        { TAG_DONE                  , 0 }
    };
    struct planarbm_data *data = OOP_INST_DATA(cl, o);
    OOP_Object *pf;

    /* First we attempt to register a pixelformat */
    pftags[0].ti_Data = bm->Depth;      /* PixFmt_Depth */
    pftags[1].ti_Data = bm->Depth;      /* PixFmt_BitsPerPixel */

    pf = DMEnum__Internal__RegisterPixFmt(CSD(cl)->dmenumclass, pftags);

    if (!pf)
    {
        /* Fail is pixelformat registration failed */
        return FALSE;
    }

    /* Free old bitmap, if it was ours. */
    PBM_FreeBitMap(data);

    /* Set the new bitmap. It's not ours. */
    data->bitmap = bm;
    data->planes_alloced = FALSE;

    /* Call private bitmap method to update superclass */
    bmtags[0].ti_Data = bm->BytesPerRow * 8;
    bmtags[1].ti_Data = bm->Rows;
    bmtags[2].ti_Data = bm->BytesPerRow;

    BM__Hidd_BitMap__SetBitMapTags(CSD(cl)->bitmapclass, o, bmtags);
    BM__Hidd_BitMap__SetPixFmt(CSD(cl)->bitmapclass, o, pf);

    return TRUE;
}

VOID PBM__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct TagItem *tag = FindTagItem(aHidd_PlanarBM_BitMap, msg->attrList);

    if (tag)
    {
        /*
         * TODO: We can't check for failure here. However, since we already
         * have Depth attribute for the bitmap, may be we shouldn't register
         * 8 pixelformats? In this case we are unable to fail.
         */
        PBM_SetBitMap(cl, obj, (struct BitMap *)tag->ti_Data);
    }

    OOP_DoSuperMethod(cl, obj, &msg->mID);
}

/****************************************************************************************/

VOID PBM__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_PutPixel *msg)
{
    UBYTE                   **plane;
    struct planarbm_data    *data;
    ULONG                   offset;
    UWORD                   mask;
    UBYTE                   pixel, notpixel;
    UBYTE                   i;
    
    data = OOP_INST_DATA(cl, o);

    if (!data->bitmap)
        return;

    /* bitmap in plane-mode */
    plane     = data->bitmap->Planes;
    offset    = msg->x / 8 + msg->y * data->bitmap->BytesPerRow;
    pixel     = 128 >> (msg->x % 8);
    notpixel  = ~pixel;
    mask      = 1;

    for (i = 0; i < data->bitmap->Depth; i++, mask <<=1, plane ++)
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

ULONG PBM__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o,
                                 struct pHidd_BitMap_GetPixel *msg)
{
    struct planarbm_data    *data;
    UBYTE                   **plane;
    ULONG                   offset;
    UWORD                   i;
    UBYTE                   pixel;
    ULONG                   retval;
         
    data = OOP_INST_DATA(cl, o);

    if (!data->bitmap)
        return 0;

    plane     = data->bitmap->Planes;
    offset    = msg->x / 8 + msg->y * data->bitmap->BytesPerRow;
    pixel     = 128 >> (msg->x % 8);
    retval    = 0;

    for (i = 0; i < data->bitmap->Depth; i++, plane ++)
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

/*
 * Fast planar FillRect: fill whole bytes per bitplane row instead of going
 * through the generic per-pixel DrawLine/PutPixel path (which is ~8x the work
 * and pays per-pixel OOP dispatch). Each plane row is set to 0x00 or 0xFF for
 * that plane's bit of the fill pen, with partial start/end bytes masked. Only
 * the plain Copy draw mode is handled here; other modes fall back to super.
 */
VOID PBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_DrawRect *msg)
{
    struct planarbm_data *data = OOP_INST_DATA(cl, o);
    struct BitMap        *bm = data->bitmap;
    HIDDT_Pixel           fg;
    HIDDT_DrawMode        mode;
    WORD                  x1, y1, x2, y2, y;
    ULONG                 firstbyte, lastbyte, rowoffset;
    UBYTE                 leftmask, rightmask;
    UBYTE                 d;

    mode = GC_DRMD(msg->gc);
    if (!bm || mode != vHidd_GC_DrawMode_Copy)
    {
        /* Rare draw modes: keep the generic (correct) implementation. */
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }

    fg = GC_FG(msg->gc);
    x1 = msg->minX; y1 = msg->minY;
    x2 = msg->maxX; y2 = msg->maxY;

    firstbyte = x1 >> 3;
    lastbyte  = x2 >> 3;
    /* Bits set within the first/last (partial) bytes; MSB is leftmost pixel. */
    leftmask  = 0xFF >> (x1 & 7);
    rightmask = 0xFF << (7 - (x2 & 7));

    for (d = 0; d < bm->Depth; d++)
    {
        UBYTE *plane = bm->Planes[d];
        UBYTE  planefill;

        if (plane == NULL || plane == (UBYTE *)-1)
            continue;

        planefill = (fg & (1 << d)) ? 0xFF : 0x00;
        rowoffset = y1 * (ULONG)bm->BytesPerRow;

        for (y = y1; y <= y2; y++, rowoffset += bm->BytesPerRow)
        {
            UBYTE *row = plane + rowoffset;

            if (firstbyte == lastbyte)
            {
                UBYTE m = leftmask & rightmask;
                row[firstbyte] = (row[firstbyte] & ~m) | (planefill & m);
            }
            else
            {
                ULONG b;

                row[firstbyte] = (row[firstbyte] & ~leftmask) | (planefill & leftmask);
                for (b = firstbyte + 1; b < lastbyte; b++)
                    row[b] = planefill;
                row[lastbyte] = (row[lastbyte] & ~rightmask) | (planefill & rightmask);
            }
        }
    }
}

/****************************************************************************************/

/*
 * In fact these two routines are implementations of C2P algorighm. The first one takes chunky
 * array of 8-bit values, the second one - 32-bit one.
 */
static void PBM_PutImage_Native(UBYTE *src, ULONG modulo, struct BitMap *data, UWORD startx, UWORD starty, UWORD width, UWORD height)
{
    ULONG planeoffset  = starty * data->BytesPerRow + startx / 8;
    UWORD x, y, d;

    startx &= 7;

    for (y = 0; y < height; y++)
    {
        UBYTE **plane = data->Planes;

        for (d = 0; d < data->Depth; d++)
        {
            UWORD dmask = 1L << d;
            UWORD pmask = 0x80 >> startx;
            UBYTE *pl = *plane;

            if (pl == (UBYTE *)-1) continue;
            if (pl == NULL) continue;

            pl += planeoffset;

            for (x = 0; x < width; x++)
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
            } /* for (x = 0; x < msg->width; x++) */

            plane++;

        } /* for (d = 0; d < data->depth; d++) */

        src         += modulo;
        planeoffset += data->BytesPerRow;
    } /* for (y = 0; y < msg->height; y++) */
}

static void PBM_PutImage_Native32(HIDDT_Pixel *src, ULONG modulo, struct BitMap *data, UWORD startx, UWORD starty, UWORD width, UWORD height)
{
    ULONG planeoffset  = starty * data->BytesPerRow + startx / 8;
    UWORD x, y, d;

    startx &= 7;

    for (y = 0; y < height; y++)
    {
        UBYTE **plane = data->Planes;

        for (d = 0; d < data->Depth; d++)
        {
            UWORD dmask = 1L << d;
            UWORD pmask = 0x80 >> startx;
            UBYTE *pl = *plane;

            if (pl == (UBYTE *)-1) continue;
            if (pl == NULL) continue;

            pl += planeoffset;

            for (x = 0; x < width; x++)
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
            } /* for (x = 0; x < msg->width; x++) */

            plane++;

        } /* for (d = 0; d < data->depth; d++) */

        src = ((APTR)src + modulo);
        planeoffset += data->BytesPerRow;
    } /* for (y = 0; y < msg->height; y++) */
}

VOID PBM__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_PutImage *msg)
{
    struct planarbm_data *data = OOP_INST_DATA(cl, o);

    if (!data->bitmap)
        return;

    if ((msg->pixFmt != vHidd_StdPixFmt_Native) &&
        (msg->pixFmt != vHidd_StdPixFmt_Native32))
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }

    switch(msg->pixFmt)
    {
    case vHidd_StdPixFmt_Native:
        PBM_PutImage_Native(msg->pixels, msg->modulo, data->bitmap, msg->x, msg->y, msg->width, msg->height);
        break;

    case vHidd_StdPixFmt_Native32:
        PBM_PutImage_Native32((HIDDT_Pixel *)msg->pixels, msg->modulo, data->bitmap, msg->x, msg->y, msg->width, msg->height);
        break;

    }
}

/****************************************************************************************/

VOID PBM__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_BitMap_PutImageLUT *msg)
{
    struct planarbm_data *data = OOP_INST_DATA(cl, o);

    if (!data->bitmap)
        return;

    /* This is the same as PutImage() with vHidd_StdPixFmt_Native format */
    PBM_PutImage_Native(msg->pixels, msg->modulo, data->bitmap, msg->x, msg->y, msg->width, msg->height);
}

/****************************************************************************************/

VOID PBM__Hidd_BitMap__GetImageLUT(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_BitMap_GetImageLUT *msg)
{
    WORD                    x, y, d;
    UBYTE                   *pixarray = (UBYTE *)msg->pixels;
    UBYTE                   **plane;
    ULONG                   planeoffset;
    struct planarbm_data    *data;
    UBYTE                   prefill;
    
    data = OOP_INST_DATA(cl, o);

    if (!data->bitmap)
        return;

    planeoffset = msg->y * data->bitmap->BytesPerRow + msg->x / 8;

    prefill = 0;
    for (d = 0; d < data->bitmap->Depth; d++)
    {
        if (data->bitmap->Planes[d] == (UBYTE *)-1)
        {
            prefill |= (1L << d);
        }
    }

    for (y = 0; y < msg->height; y++)
    {
        UBYTE *dest = pixarray;

        plane = data->bitmap->Planes;
        for(x = 0; x < msg->width; x++)
        {
            dest[x] = prefill;
        }
        
        for (d = 0; d < data->bitmap->Depth; d++)
        {
            UWORD dmask = 1L << d;
            UWORD pmask = 0x80 >> (msg->x & 7);
            UBYTE *pl = *plane;

            if (pl == (UBYTE *)-1) continue;
            if (pl == NULL) continue;

            pl += planeoffset;

            for (x = 0; x < msg->width; x++)
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
        
        pixarray    += msg->modulo;
        planeoffset += data->bitmap->BytesPerRow;
        
    } /* for(y = 0; y < msg->height; y++) */
    
}

/****************************************************************************************/

BOOL PBM__Hidd_PlanarBM__SetBitMap(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_PlanarBM_SetBitMap *msg)
{
    return PBM_SetBitMap(cl, o, msg->bitMap);
}

/****************************************************************************************/

BOOL PBM__Hidd_PlanarBM__GetBitMap(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_PlanarBM_GetBitMap *msg)
{
    struct planarbm_data *data = OOP_INST_DATA(cl, o);

    if (!data->bitmap)
        return FALSE;

    /*
     * Totally obsolete and deprecated.
     * Just get aoHidd_PlanarBM_BitMap value instead.
     */
    CopyMem(data->bitmap, msg->bitMap, sizeof(struct BitMap));
    return TRUE;
}

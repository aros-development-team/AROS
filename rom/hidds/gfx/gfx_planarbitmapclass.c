/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

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

#ifdef __mc68000
#include <hardware/custom.h>
#include <proto/graphics.h>
#endif

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
    BOOL interleaved;
    BOOL ok = FALSE;
    struct planarbm_data *data;
    struct TagItem *tag;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);
    if (NULL == o)
        return NULL;

    data = OOP_INST_DATA(cl, o);
    data->cached_depth_plus_one = 0;
    data->cache_pixfmt = FALSE;

    /* Check if we want to use existing bitmap */

    tag = FindTagItem(aHidd_PlanarBM_BitMap, msg->attrList);
    if (tag)
    {
        /* graphics.library's reusable wrapper starts with no BitMap. It may
         * retain the independent pixel-format reference while detached. */
        data->cache_pixfmt = (tag->ti_Data == 0);
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
    interleaved = GetTagData(aHidd_PlanarBM_Interleaved, FALSE,
                             msg->attrList);

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
        ULONG planesize = height * bytesperrow;

        if (interleaved && depth)
        {
            PLANEPTR planes = AllocMem(planesize * depth, planeflags);

            if (planes)
            {
                for (i = 0; i < depth; i++)
                    data->bitmap->Planes[i] = planes + i * bytesperrow;

                data->bitmap->BytesPerRow *= depth;
                data->bitmap->Flags |= BMF_INTERLEAVED;
            }
            else
            {
                /* AllocBitMap() permits falling back to separate planes. */
                interleaved = FALSE;
            }
        }

        if (!interleaved)
        {
            for (i = 0; i < depth; i++)
            {
                data->bitmap->Planes[i] = AllocMem(planesize, planeflags);

                if (NULL == data->bitmap->Planes[i])
                {
                    D(bug("[PlanarBM] %s: plane %d allocation failed (%lu bytes, flags 0x%lx)\n",
                          __func__, i, (unsigned long)planesize,
                          (unsigned long)planeflags));
                    ok = FALSE;
                    break;
                }
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

            if (data->bitmap->Flags & BMF_INTERLEAVED)
            {
                if (data->bitmap->Planes[0])
                {
                    FreeMem(data->bitmap->Planes[0],
                            data->bitmap->Rows * data->bitmap->BytesPerRow);
                }
            }
            else
            {
                for (i = 0; i < data->bitmap->Depth; i++)
                {
                    if (data->bitmap->Planes[i])
                    {
                        FreeMem(data->bitmap->Planes[i],
                                data->bitmap->Rows * data->bitmap->BytesPerRow);
                    }
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
    struct planarbm_data *data = OOP_INST_DATA(cl, o);
    struct HIDDBitMapData *bmdata = OOP_INST_DATA(CSD(cl)->bitmapclass, o);
    OOP_Object *pf = NULL;
    BOOL reuse_pf;

    /* Detach from the caller-owned bitmap. A graphics cache wrapper keeps
     * only its own registered pixel format, never the BitMap or its planes. */
    if (!bm)
    {
        PBM_FreeBitMap(data);
        data->bitmap = NULL;
        data->planes_alloced = FALSE;

        bmdata->width = 0;
        bmdata->height = 0;
        bmdata->bytesPerRow = 0;
        if (!data->cache_pixfmt)
        {
            BM__Hidd_BitMap__SetPixFmt(CSD(cl)->bitmapclass, o, NULL);
            data->cached_depth_plus_one = 0;
        }
        return TRUE;
    }

    reuse_pf = data->cache_pixfmt &&
               data->cached_depth_plus_one == (UWORD)bm->Depth + 1;
    if (!reuse_pf)
    {
        pftags[0].ti_Data = bm->Depth;
        pftags[1].ti_Data = bm->Depth;
        pf = DMEnum__Internal__RegisterPixFmt(CSD(cl)->dmenumclass, pftags);
        if (!pf)
            return FALSE;
    }

    /* Free old bitmap, if it was ours. */
    PBM_FreeBitMap(data);

    /* Set the new bitmap. It's not ours. */
    data->bitmap = bm;
    data->planes_alloced = FALSE;

    /* These are the only three attributes SetBitMapTags would update here;
     * setting the superclass data directly avoids parsing a tag list twice
     * for every short-lived render wrapper. */
    bmdata->width = bm->BytesPerRow * 8;
    if ((bm->Flags & BMF_INTERLEAVED) && bm->Depth)
        bmdata->width /= bm->Depth;
    bmdata->height = bm->Rows;
    bmdata->bytesPerRow = bm->BytesPerRow;

    if (!reuse_pf)
    {
        BM__Hidd_BitMap__SetPixFmt(CSD(cl)->bitmapclass, o, pf);
        data->cached_depth_plus_one = (UWORD)bm->Depth + 1;
    }

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

VOID PBM__Hidd_BitMap__PutTemplate(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_BitMap_PutTemplate *msg)
{
#ifdef __mc68000
    struct planarbm_data *data = OOP_INST_DATA(cl, o);
    struct BitMap *bm = data->bitmap;
    struct GfxBase *GfxBase = CSD(cl)->cs_GfxBase;
    volatile struct Custom *custom = (struct Custom *)0xdff000;
    ULONG src_offset, dst_offset, bitmap_width;
    ULONG fg, bg, colmask;
    WORD src_x2, dst_x2, src_width, dst_width, width;
    WORD src_x, dst_x, shift;
    UWORD first_mask, last_mask, shift_a, shift_b;
    BOOL transparent, invert;
    BOOL reverse;
    UBYTE plane;

    if (!bm || !msg->masktemplate || msg->width <= 0 || msg->height <= 0)
        return;

    bitmap_width = (ULONG)bm->BytesPerRow * 8;
    if ((bm->Flags & BMF_INTERLEAVED) && bm->Depth)
        bitmap_width /= bm->Depth;

    /* Use the blitter only when every DMA source and destination is in Chip
     * RAM. Leave other formats and out-of-range direct HIDD calls to the
     * superclass implementation. */
    if (msg->srcx < 0 || msg->x < 0 || msg->y < 0 ||
        !bm->Depth || bm->Depth > 8 ||
        (ULONG)msg->x + msg->width > bitmap_width ||
        (ULONG)msg->y + msg->height > bm->Rows ||
        !(TypeOfMem(msg->masktemplate) & MEMF_CHIP))
        goto software;

    for (plane = 0; plane < bm->Depth; plane++)
    {
        UBYTE *bits = bm->Planes[plane];
        if (bits && bits != (UBYTE *)-1 && !(TypeOfMem(bits) & MEMF_CHIP))
            goto software;
    }

    src_x = msg->srcx;
    dst_x = msg->x;
    src_x2 = src_x + msg->width - 1;
    dst_x2 = dst_x + msg->width - 1;
    src_width = src_x2 / 16 - src_x / 16 + 1;
    dst_width = dst_x2 / 16 - dst_x / 16 + 1;
    shift = (dst_x & 15) - (src_x & 15);
    reverse = shift < 0;
    if (reverse)
        shift = -shift;

    width = src_width > dst_width ? src_width : dst_width;
    if (!(GfxBase->ChipRevBits0 & GFXF_BIG_BLITS) &&
        (width > 64 || msg->height > 1024))
        goto software;

    src_offset = (src_x / 16) * 2;
    dst_offset = bm->BytesPerRow * msg->y + (dst_x / 16) * 2;
    src_x &= 15;
    dst_x &= 15;
    src_x2 &= 15;
    dst_x2 &= 15;

    if (reverse)
    {
        shift_a = dst_width >= src_width ? 0 : shift << 12;
        first_mask = dst_width >= src_width
            ? (UWORD)(0xffff << (15 - dst_x2))
            : (UWORD)(0xffff << (15 - src_x2));
        last_mask = dst_width >= src_width
            ? (UWORD)(0xffff >> dst_x)
            : (UWORD)(0xffff >> src_x);
        src_offset += msg->modulo * (msg->height - 1) + (width - 1) * 2;
        dst_offset += bm->BytesPerRow * (msg->height - 1) + (width - 1) * 2;
    }
    else
    {
        shift_a = dst_width >= src_width ? 0 : shift << 12;
        first_mask = dst_width >= src_width
            ? (UWORD)(0xffff >> dst_x)
            : (UWORD)(0xffff >> src_x);
        last_mask = dst_width >= src_width
            ? (UWORD)(0xffff << (15 - dst_x2))
            : (UWORD)(0xffff << (15 - src_x2));
    }
    shift_b = shift << 12;

    fg = GC_FG(msg->gc);
    bg = GC_BG(msg->gc);
    colmask = GC_COLMASK(msg->gc);
    transparent = GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Transparent;
    invert = !transparent && GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Invert;

    OwnBlitter();
    WaitBlit();
    custom->bltafwm = first_mask;
    custom->bltalwm = last_mask;
    custom->bltbmod = msg->modulo - width * 2;
    custom->bltcmod = bm->BytesPerRow - width * 2;
    custom->bltdmod = bm->BytesPerRow - width * 2;
    custom->bltadat = 0xffff;

    for (plane = 0; plane < bm->Depth; plane++)
    {
        UBYTE *bits = bm->Planes[plane];
        ULONG plane_bit = 1UL << plane;
        UBYTE minterm = 0x0a; /* Preserve C where the A edge mask is zero. */

        if (!(colmask & plane_bit) || !bits || bits == (UBYTE *)-1)
            continue;

        if (transparent)
        {
            minterm |= msg->inverttemplate ? 0x80 : 0x20;
            if (fg & plane_bit)
                minterm |= msg->inverttemplate ? 0x30 : 0xc0;
        }
        else if (invert)
            minterm |= msg->inverttemplate ? 0x90 : 0x60;
        else
        {
            if ((msg->inverttemplate ? bg : fg) & plane_bit)
                minterm |= 0xc0;
            if ((msg->inverttemplate ? fg : bg) & plane_bit)
                minterm |= 0x30;
        }

        WaitBlit();
        custom->bltcon0 = shift_a | 0x0700 | minterm;
        custom->bltcon1 = (reverse ? 2 : 0) | shift_b;
        custom->bltbdat = 0xffff;
        custom->bltbpt = msg->masktemplate + src_offset;
        custom->bltcpt = bits + dst_offset;
        custom->bltdpt = bits + dst_offset;
        if (GfxBase->ChipRevBits0 & GFXF_BIG_BLITS)
        {
            custom->bltsizv = msg->height;
            custom->bltsizh = width;
        }
        else
            custom->bltsize = (msg->height << 6) | (width & 63);
    }

    WaitBlit();
    DisownBlitter();
    return;

software:
#endif
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
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

#ifdef __mc68000
    /* RectFill on an off-screen Chip bitmap must not fall back to a CPU
     * byte loop.  In particular, CD32 titles repeatedly clear narrow
     * status bands before redrawing their text.  Keep the software path for
     * non-Chip planes, clipped/out-of-range HIDD calls, and other modes. */
    if (bm->Depth && bm->Depth <= 8 &&
        (GC_COLMASK(msg->gc) & ((1UL << bm->Depth) - 1)) ==
            ((1UL << bm->Depth) - 1) &&
        x1 >= 0 && y1 >= 0 && x2 >= x1 && y2 >= y1 &&
        y2 < bm->Rows)
    {
        ULONG bitmap_width = (ULONG)bm->BytesPerRow * 8;
        UWORD words = x2 / 16 - x1 / 16 + 1;
        UWORD lines = y2 - y1 + 1;
        BOOL chip_planes = TRUE;

        if (bm->Flags & BMF_INTERLEAVED)
            bitmap_width /= bm->Depth;

        if ((ULONG)x2 < bitmap_width && words <= 63 && lines <= 1023)
        {
            for (d = 0; d < bm->Depth; d++)
            {
                UBYTE *plane = bm->Planes[d];
                if (plane && plane != (UBYTE *)-1 &&
                    !(TypeOfMem(plane) & MEMF_CHIP))
                {
                    chip_planes = FALSE;
                    break;
                }
            }

            if (chip_planes)
            {
                struct GfxBase *GfxBase = CSD(cl)->cs_GfxBase;
                volatile struct Custom *custom = (struct Custom *)0xdff000;
                ULONG offset = (ULONG)y1 * bm->BytesPerRow + (x1 / 16) * 2;

                OwnBlitter();
                WaitBlit();
                custom->bltafwm = (UWORD)(0xffff >> (x1 & 15));
                custom->bltalwm = (UWORD)(0xffff << (15 - (x2 & 15)));
                custom->bltcmod = bm->BytesPerRow - words * 2;
                custom->bltdmod = bm->BytesPerRow - words * 2;
                custom->bltcon1 = 0;
                custom->bltcon0 = 0x0300 | 0xca; /* A ? B : C */
                custom->bltadat = 0xffff;

                for (d = 0; d < bm->Depth; d++)
                {
                    UBYTE *plane = bm->Planes[d];
                    if (!plane || plane == (UBYTE *)-1)
                        continue;

                    WaitBlit();
                    custom->bltbdat = (fg & (1UL << d)) ? 0xffff : 0;
                    custom->bltcpt = plane + offset;
                    custom->bltdpt = plane + offset;
                    custom->bltsize = (lines << 6) | words;
                }
                WaitBlit();
                DisownBlitter();
                return;
            }
        }
    }
#endif

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

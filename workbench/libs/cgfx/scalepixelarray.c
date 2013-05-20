/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <hidd/graphics.h>
#include <proto/cybergraphics.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

struct render_data
{
    OOP_Object *srcbm_obj;
    OOP_Object *gfx_hidd;
};

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH10(LONG, ScalePixelArray,

/*  SYNOPSIS */
	AROS_LHA(APTR             , srcRect, A0),
	AROS_LHA(UWORD            , SrcW, D0),
	AROS_LHA(UWORD            , SrcH, D1),
	AROS_LHA(UWORD            , SrcMod, D2),
	AROS_LHA(struct RastPort *, RastPort, A1),
	AROS_LHA(UWORD            , DestX, D3),
	AROS_LHA(UWORD            , DestY, D4),
	AROS_LHA(UWORD            , DestW, D5),
	AROS_LHA(UWORD            , DestH, D6),
	AROS_LHA(UBYTE            , SrcFormat, D7),

/*  LOCATION */
	struct Library *, CyberGfxBase, 15, Cybergraphics)

/*  FUNCTION
        Fills all or part of a RastPort with a rectangular block of raw pixel
        values. The source pixels are scaled to fit the destination area, i.e.
        some pixels may be duplicated or dropped according to the need to
        stretch or compress the source block.

    INPUTS
        srcRect - pointer to the pixel values.
        SrcW, SrcH - width and height of the source rectangle (in pixels).
        SrcMod - the number of bytes in each row of the source rectangle.
        RastPort - the RastPort to write to.
        DestX, DestY - top-lefthand corner of portion of destination RastPort
            to write to (in pixels).
        DestW, DestH - size of the destination rectangle (in pixels).
        SrcFormat - the format of the source pixels. See WritePixelArray for
            possible values.

    RESULT
        count - the number of pixels written to.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        WritePixelArray()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG result = 0;
    struct render_data data;
    struct Rectangle rr;
    OOP_Object *gfx_hidd, *tempbm_obj, *tempbm2_obj, *gc;
    struct BitScaleArgs scale_args = {0};
    struct TagItem bm_tags[] =
    {
        {aHidd_BitMap_GfxHidd, 0},
        {aHidd_BitMap_Width, SrcW},
        {aHidd_BitMap_Height, SrcH},
        {aHidd_BitMap_StdPixFmt, 0},
        {TAG_END, 0}
    };
    struct TagItem gc_tags[] =
    {
        {aHidd_GC_DrawMode, vHidd_GC_DrawMode_Copy},
        {TAG_DONE, 0UL}
    };

    D(bug("ScalePixelArray(%p, %d, %d, %d, %p, %d, %d, %d, %d, %d)\n",
        srcRect, SrcW, SrcH, SrcMod, RastPort, DestX, DestY, DestW, DestH,
        SrcFormat));

    if (SrcW == 0 || SrcH == 0 || DestW == 0 || DestH == 0)
    	return 0;

    /* This is cybergraphx. We only work wih HIDD bitmaps */

    if (!IS_HIDD_BM(RastPort->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in ScalePixelArray() !!!\n"));
    	return 0;
    }

    /* Get graphics HIDD and a graphics context */

    OOP_GetAttr(HIDD_BM_OBJ(RastPort->BitMap), aHidd_BitMap_GfxHidd,
        (IPTR *)&gfx_hidd);
    gc = HIDD_Gfx_NewGC(gfx_hidd, gc_tags);

    /* Create two temporary bitmap objects: one the size of the source area
       and another the size of the destination area */

    bm_tags[0].ti_Data = (IPTR)gfx_hidd;
    bm_tags[3].ti_Data = GetHIDDRectFmt(SrcFormat, RastPort, CyberGfxBase);
    tempbm_obj = HIDD_Gfx_NewBitMap(gfx_hidd, bm_tags);

    bm_tags[1].ti_Data = DestW;
    bm_tags[2].ti_Data = DestH;
#if 0
    // FIXME: This doesn't work (X11 and VESA). Should it?
    bm_tags[3].ti_Tag = aHidd_BitMap_Friend;
    bm_tags[3].ti_Data = (IPTR)HIDD_BM_OBJ(RastPort->BitMap);
#endif
    tempbm2_obj = HIDD_Gfx_NewBitMap(gfx_hidd, bm_tags);

    if (gc != NULL && tempbm_obj != NULL && tempbm2_obj != NULL)
    {
        /* Copy the source array to its temporary bitmap object */

        HIDD_BM_PutImage(tempbm_obj, gc, srcRect, SrcMod, 0, 0, SrcW, SrcH,
            vHidd_StdPixFmt_Native);

        /* Scale temporary source bitmap on to temporary destination bitmap */

        scale_args.bsa_SrcWidth = SrcW;
        scale_args.bsa_SrcHeight = SrcH;
        scale_args.bsa_DestWidth = DestW;
        scale_args.bsa_DestHeight = DestH;
        HIDD_BM_BitMapScale(tempbm2_obj, tempbm_obj, tempbm2_obj, &scale_args,
            gc);

        /* Render temporary destination bitmap to destination bitmap */

        data.srcbm_obj = tempbm2_obj;
        data.gfx_hidd = gfx_hidd;
        rr.MinX = DestX;
        rr.MinY = DestY;
        rr.MaxX = DestX + DestW - 1;
        rr.MaxY = DestY + DestH - 1;
        result = DoRenderFunc(RastPort, NULL, &rr, RenderHook, &data, TRUE);
    }

    /* Discard temporary resources */

    OOP_DisposeObject(tempbm_obj);
    OOP_DisposeObject(tempbm2_obj);
    OOP_DisposeObject(gc);

    return result;

    AROS_LIBFUNC_EXIT
} /* ScalePixelArray */

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase)
{
    ULONG  width  = rect->MaxX - rect->MinX + 1;
    ULONG  height = rect->MaxY - rect->MinY + 1;

    HIDD_Gfx_CopyBox(data->gfx_hidd, data->srcbm_obj, srcx, srcy, dstbm_obj,
        rect->MinX, rect->MinY, width, height, dst_gc);

    return width * height;
}


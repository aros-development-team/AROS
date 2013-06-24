/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <hidd/graphics.h>
#include <aros/debug.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

#include <proto/utility.h>

#include <clib/macros.h>

struct render_data
{
    struct CDrawMsg msg;
    OOP_Object *pf;
    struct Hook *hook;
    struct RastPort *rp;
    IPTR stdpf;
    struct IntCGFXBase *CyberGfxBase;
};


static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH3(void, DoCDrawMethodTagList,

/*  SYNOPSIS */
	AROS_LHA(struct Hook     *, hook, A0),
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(struct TagItem  *, tags, A2),

/*  LOCATION */
	struct Library *, CyberGfxBase, 26, Cybergraphics)

/*  FUNCTION
        Calls a callback hook that directly accesses a RastPort's bitmap.

    INPUTS
        hook - a callback hook. The standard hook inputs will be set as
            follows:
                object (struct RastPort *) - this function's 'rp' input.
                message (struct CDrawMsg *) - details of the area on which to
                    operate.
        rp - the RastPort to perform operations on.
        tags - not used. Must be NULL.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct render_data data;
    struct Rectangle rr;
    struct Layer *L;

    if (!IS_HIDD_BM(rp->BitMap))
    {
        D(bug("!!! NO HIDD BITMAP IN CALL TO DoCDrawMethodTagList() !!!\n"));
        return;
    }

    /* Get the bitmap std pixfmt */
    OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_PixFmt,
        (IPTR *) & data.pf);
    OOP_GetAttr(data.pf, aHidd_PixFmt_StdPixFmt, &data.stdpf);
    data.msg.cdm_ColorModel = OOP_GET(data.pf, aHidd_PixFmt_CgxPixFmt);
    data.hook = hook;
    data.rp = rp;

    if (((UWORD) - 1) == data.msg.cdm_ColorModel)
    {
        D(bug("!!! UNKNOWN HIDD PIXFMT IN DoCDrawMethodTagList() !!!\n"));
        return;
    }

    L = rp->Layer;

    rr.MinX = 0;
    rr.MinY = 0;

    if (NULL == L)
    {
        rr.MaxX = GetBitMapAttr(rp->BitMap, BMA_WIDTH) - 1;
        rr.MaxY = GetBitMapAttr(rp->BitMap, BMA_HEIGHT) - 1;
    }
    else
    {
        /* Lock the layer */
        LockLayerRom(L);

        rr.MaxX = rr.MinX + (L->bounds.MaxX - L->bounds.MinX) - 1;
        rr.MaxY = rr.MinY + (L->bounds.MaxY - L->bounds.MinY) - 1;
    }

    data.CyberGfxBase = GetCGFXBase(CyberGfxBase);
    DoRenderFunc(rp, NULL, &rr, RenderHook, &data, TRUE);

    if (NULL != L)
    {
        UnlockLayerRom(L);
    }

    return;

    AROS_LIBFUNC_EXIT
} /* DoCDrawMethodTagList */

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object * dstbm_obj, OOP_Object * dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase)
{
    struct IntCGFXBase *CyberGfxBase = data->CyberGfxBase;
    struct CDrawMsg *msg = &data->msg;
    ULONG width = rect->MaxX - rect->MinX + 1;
    ULONG height = rect->MaxY - rect->MinY + 1;
    UBYTE *addr;
    IPTR bytesperpixel, bytesperrow;
    ULONG fb_width, fb_height;
    ULONG banksize, memsize;

#if 1
    msg->cdm_offx = rect->MinX;
    msg->cdm_offy = rect->MinY;
#else
#warning "Not sure about this one. Set it to 0 since we adjust msg->cdm_MemPtr to x1/y1 lower down"
    msg->cdm_offx = 0;          // x1;
    msg->cdm_offy = 0;          // y1;
#endif
    msg->cdm_xsize = width;
    msg->cdm_ysize = height;

    OOP_GetAttr(data->pf, aHidd_PixFmt_BytesPerPixel, &bytesperpixel);
    OOP_GetAttr(dstbm_obj, aHidd_BitMap_BytesPerRow, &bytesperrow);

    D(kprintf("width %d bytesperrow %d bytesperpixel %d\n", width,
            bytesperrow, bytesperpixel));
    D(kprintf(" colormodel %d\n", msg->cdm_ColorModel));

    /* Get the baseadress from where to render */
    if (HIDD_BM_ObtainDirectAccess(dstbm_obj, &addr, &fb_height, &fb_width,
            &banksize, &memsize))
    {
        msg->cdm_BytesPerPix = (UWORD) bytesperpixel;
        msg->cdm_BytesPerRow = bytesperrow;
        /* Colormodel already set */

        /* Compute the address for the start pixel */
#if 1
        msg->cdm_MemPtr = addr;
#else
        msg->cdm_MemPtr =
            addr + (msg->bytesperrow * y1) + (bytesperpixel * x1);
#endif

        HIDD_BM_ReleaseDirectAccess(dstbm_obj);

        CallHookPkt(data->hook, data->rp, msg);
    }
    else
    {
        /* We are unable to gain direct access to the framebuffer,
           so we have to emulate it
         */
        struct TagItem gc_tags[] =
        {
            {aHidd_GC_DrawMode, vHidd_GC_DrawMode_Copy},
            {TAG_DONE, 0UL}
        };
        ULONG tocopy_h, max_tocopy_h;
        ULONG lines_todo;
        OOP_Object *gfxhidd, *gc;

        lines_todo = height;

        /* The HIDD bm does not have a base adress so we have to render into
           it using a temporary buffer
         */

        if (PIXELBUF_SIZE < bytesperrow)
        {
            D(bug("!!! NOT ENOUGH SPACE IN TEMP BUFFER FOR A SINGLE LINE "
                "IN DoCDrawMethodTagList() !!!\n"));
            return 0;
        }

        /* Calculate number of lines we might copy */
        max_tocopy_h = PIXELBUF_SIZE / bytesperrow;

#if 1
        msg->cdm_offx = 0;
        msg->cdm_offy = 0;
#endif

        OOP_GetAttr(dstbm_obj, aHidd_BitMap_GfxHidd, (IPTR *) & gfxhidd);
        gc = HIDD_Gfx_NewGC(gfxhidd, gc_tags);

        /* Get the maximum number of lines */
        while (lines_todo != 0)
        {
            tocopy_h = MIN(lines_todo, max_tocopy_h);
            msg->cdm_MemPtr = CyberGfxBase->pixel_buf;
            msg->cdm_BytesPerRow = bytesperrow;

            msg->cdm_BytesPerPix = (UWORD) bytesperpixel;

            LOCK_PIXBUF
            HIDD_BM_GetImage(dstbm_obj, (UBYTE *) CyberGfxBase->pixel_buf,
                bytesperrow, rect->MinX, rect->MinY + height - lines_todo,
                width, lines_todo, data->stdpf);

            /* Use the hook to set some pixels */
            CallHookPkt(data->hook, data->rp, msg);

            HIDD_BM_PutImage(dstbm_obj, gc,
                (UBYTE *) CyberGfxBase->pixel_buf, bytesperrow, rect->MinX,
                rect->MinY + height - lines_todo, width, lines_todo,
                data->stdpf);

            ULOCK_PIXBUF

            lines_todo -= tocopy_h;
        }

        OOP_DisposeObject(gc);
    }

    return width * height;
}

/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "x11_debug.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <aros/macros.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/graphics.h>
#include <oop/oop.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include "x11.h"
#include "bitmap.h"
#include "x11gfx_intern.h"

/****************************************************************************************/

#define DO_ENDIAN_FIX     1 /* fix if X11 server running on remote server with different endianess */

/****************************************************************************************/

#define MNAME(x) X11BM__ ## x

#if DO_ENDIAN_FIX

#if AROS_BIG_ENDIAN
#define NEEDS_ENDIAN_FIX(image) (((image)->bits_per_pixel >= 15) && ((image)->byte_order != MSBFirst))
#define SWAP16(x) AROS_WORD2LE(x)
#define SWAP32(x) AROS_LONG2LE(x)
#define AROS_BYTEORDER MSBFirst
#else
#define NEEDS_ENDIAN_FIX(image) (((image)->bits_per_pixel >= 15) && ((image)->byte_order != LSBFirst))
#define SWAP16(x) AROS_WORD2BE(x)
#define SWAP32(x) AROS_LONG2BE(x)
#define AROS_BYTEORDER LSBFirst
#endif

#if 0 /* stegerg: to test above stuff*/
#define NEEDS_ENDIAN_FIX(image) ((image)->bits_per_pixel >= 15)
#define AROS_BYTEORDER MSBFirst
#define SWAP16(x) AROS_WORD2BE(x)
#define SWAP32(x) AROS_LONG2BE(x)
#endif

/****************************************************************************************/

OOP_Object *X11BM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    o = (OOP_Object *) OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct bitmap_data *data = OOP_INST_DATA(cl, o);
        BOOL ok = TRUE;
        BOOL framebuffer;

        /* Get some info passed to us by the x11gfxhidd class */
        data->display = (Display *) GetTagData(aHidd_X11BitMap_SysDisplay, 0, msg->attrList);
        data->screen = GetTagData(aHidd_X11BitMap_SysScreen, 0, msg->attrList);
        data->cursor = (Cursor) GetTagData(aHidd_X11BitMap_SysCursor, 0, msg->attrList);
        data->colmap = (Colormap) GetTagData(aHidd_X11BitMap_ColorMap, 0, msg->attrList);
        framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

        D(
            bug("[X11Bm] %s: display @ 0x%p, screen #%d\n", __PRETTY_FUNCTION__, data->display, data->screen);
            bug("[X11Bm] %s: cursor @ 0x%p, colormap @ 0x%p\n", __PRETTY_FUNCTION__, data->cursor, data->colmap);
        )

        if (framebuffer)
        {
            /* Framebuffer is X11 window */
            data->flags |= BMDF_FRAMEBUFFER;
            ok = X11BM_InitFB(cl, o, msg->attrList);
        }
        else
        {
            /* Anything else is a pixmap */
            ok = X11BM_InitPM(cl, o, msg->attrList);
        }

        if (ok)
        {
            /* Create an X11 GC. All objects need it. */
            XGCValues gcval;

            gcval.plane_mask = AllPlanes;
            gcval.graphics_exposures = False;

            HostLib_Lock();

            data->gc = XCALL(XCreateGC, data->display, DRAWABLE(data),
                    GCPlaneMask | GCGraphicsExposures, &gcval);
            HostLib_Unlock();

            if (!data->gc)
                ok = FALSE;
#if X11SOFTMOUSE
            else if (framebuffer)
                X11BM_InitEmptyCursor(data);
#endif
        }

        if (!ok)
        {
            OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

            OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
            o = NULL;
        }
    } /* if (object allocated by superclass) */

    D(bug("[X11Bm] %s: returning object @ 0x%p\n", __PRETTY_FUNCTION__, o));
    return o;
}

/****************************************************************************************/

VOID X11BM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    if (data->gc)
    {
        HostLib_Lock();
        XCALL(XFreeGC, data->display, data->gc);
        HostLib_Unlock();
    }

    if (data->flags & BMDF_FRAMEBUFFER)
        X11BM_DisposeFB(data, XSD(cl));
    else
        X11BM_DisposePM(data);

    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

VOID X11BM__Hidd_BitMap__Clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    HostLib_Lock();

    if (data->flags & BMDF_FRAMEBUFFER)
        X11BM_ClearFB(data, GC_BG(msg->gc));
    else
        X11BM_ClearPM(data, GC_BG(msg->gc));

    HostLib_Unlock();
}

/****************************************************************************************/

static void SwapImageEndianess(XImage *image)
{
    LONG x, y, height, width, bpp;
    UBYTE *imdata = (UBYTE *) image->data;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    width = image->width;
    height = image->height;
    bpp = (image->bits_per_pixel + 7) / 8;

    for (y = 0; y < height; y++)
    {
        switch (bpp)
        {
        case 2:
            for (x = 0; x < width; x++, imdata += 2)
            {
                UWORD pix = *(UWORD *) imdata;

                pix = SWAP16(pix);

                *(UWORD *) imdata = pix;
            }
            imdata += (image->bytes_per_line - width * 2);
            break;

        case 3:
            for (x = 0; x < width; x++, imdata += 3)
            {
                UBYTE pix1 = imdata[0];
                UBYTE pix3 = imdata[2];

                imdata[0] = pix3;
                imdata[2] = pix1;
            }
            imdata += (image->bytes_per_line - width * 3);
            break;

        case 4:
            for (x = 0; x < width; x++, imdata += 4)
            {
                ULONG pix = *(ULONG *) imdata;

                pix = SWAP32(pix);

                *(ULONG *) imdata = pix;
            }
            imdata += (image->bytes_per_line - width * 4);
            break;

        } /* switch(bpp) */

    } /* for (y = 0; y < height; y ++) */

    image->byte_order = AROS_BYTEORDER;
}

/****************************************************************************************/

#endif /* DO_ENDIAN_FIX */

/****************************************************************************************/

BOOL MNAME(Hidd_BitMap__SetColors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat *pf;
    ULONG xc_i, col_i;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    if (!OOP_DoSuperMethod(cl, o, &msg->mID))
        return FALSE;

    pf = BM_PIXFMT(o);

    if (vHidd_ColorModel_StaticPalette == HIDD_PF_COLMODEL(pf) || vHidd_ColorModel_TrueColor == HIDD_PF_COLMODEL(pf))
    {
        /* Superclass has taken care of this case */
        return TRUE;
    }

    /* Ve have a vHidd_GT_Palette bitmap */

    if (data->flags & BMDF_COLORMAP_ALLOCED)
    {
        LOCK_X11

        for (xc_i = msg->firstColor, col_i = 0; col_i < msg->numColors; xc_i++, col_i++)
        {
            XColor xcol;

            xcol.red = msg->colors[col_i].red;
            xcol.green = msg->colors[col_i].green;
            xcol.blue = msg->colors[col_i].blue;
            xcol.pad = 0;
            xcol.pixel = xc_i;
            xcol.flags = DoRed | DoGreen | DoBlue;

            XCALL(XStoreColor, data->display, data->colmap, &xcol);

        }

        UNLOCK_X11

    } /* if (data->flags & BMDF_COLORMAP_ALLOCED) */

    return TRUE;
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__PutPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    LOCK_X11

    XCALL(XSetForeground, data->display, data->gc, msg->pixel);
    XCALL(XSetFunction, data->display, data->gc, GXcopy);
    XCALL(XDrawPoint, data->display, DRAWABLE(data), data->gc, msg->x, msg->y);

    UNLOCK_X11
}

/****************************************************************************************/

HIDDT_Pixel MNAME(Hidd_BitMap__GetPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel pixel = -1;
    XImage *image;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    LOCK_X11

    XCALL(XSync, data->display, False);

    image = XCALL(XGetImage, data->display, DRAWABLE(data), msg->x, msg->y, 1, 1, AllPlanes, ZPixmap);

    if (image)
    {
        pixel = XGetPixel(image, 0, 0);
        XDestroyImage(image);
    }

    UNLOCK_X11

    return pixel;

}

/****************************************************************************************/

ULONG MNAME(Hidd_BitMap__DrawPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    XGCValues gcval;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    gcval.function = GC_DRMD(msg->gc);
    gcval.foreground = GC_FG(msg->gc);
    gcval.background = GC_BG(msg->gc);

    LOCK_X11
    XCALL(XChangeGC, data->display, data->gc, GCFunction | GCForeground | GCBackground, &gcval);
    XCALL(XDrawPoint, data->display, DRAWABLE(data), data->gc, msg->x, msg->y);
    UNLOCK_X11

    return 0;
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__FillRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    XGCValues gcval;

    D(bug("[X11Bm] %s(%d,%d,%d,%d)\n", __PRETTY_FUNCTION__, msg->minX, msg->minY, msg->maxX, msg->maxY));
    D(bug("[X11Bm] %s: Drawmode = %d\n", __PRETTY_FUNCTION__, GC_DRMD(msg->gc)));

    gcval.function = GC_DRMD(msg->gc);
    gcval.foreground = GC_FG(msg->gc);
    gcval.background = GC_BG(msg->gc);

    LOCK_X11
    XCALL(XChangeGC, data->display, data->gc, GCFunction | GCForeground | GCBackground, &gcval);

    XCALL(XFillRectangle, data->display, DRAWABLE(data), data->gc, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1);

    UNLOCK_X11
}

/****************************************************************************************/

static ULONG *ximage_to_buf(OOP_Class *cl, OOP_Object *bm, HIDDT_Pixel *buf, XImage *image, ULONG width, ULONG height,
        ULONG depth, struct pHidd_BitMap_GetImage *msg)
{
    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    switch (msg->pixFmt)
    {
    case vHidd_StdPixFmt_Native:
    {
        UBYTE *imdata = image->data;
        LONG y;

        for (y = 0; y < height; y++)
        {
            memcpy(buf, imdata, msg->width * image->bits_per_pixel / 8);

            imdata += image->bytes_per_line;
            buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo);
        }
    }
        break;

    case vHidd_StdPixFmt_Native32:
        switch (image->bits_per_pixel)
        {
        case 8:
        {
            UBYTE *imdata = (UBYTE *) image->data;
            LONG x, y;

            for (y = 0; y < height; y++)
            {
                HIDDT_Pixel *p = buf;

                for (x = 0; x < width; x++)
                {
                    *p++ = *imdata++;
                }
                imdata += (image->bytes_per_line - width);
                buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo);
            }
            break;
        }

        case 16:
        {
            UWORD *imdata = (UWORD *) image->data;
            LONG x, y;

            for (y = 0; y < height; y++)
            {
                HIDDT_Pixel *p = buf;

                for (x = 0; x < width; x++)
                {
                    *p++ = *imdata++;
                }
                imdata += image->bytes_per_line / 2 - width;
                buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo);
            }
            break;
        }

        case 32:
        {
            ULONG *imdata = (ULONG *) image->data;
            LONG x, y;

            for (y = 0; y < height; y++)
            {
                HIDDT_Pixel *p = buf;

                for (x = 0; x < width; x++)
                {
                    *p++ = *imdata++;
                }
                imdata += image->bytes_per_line / 4 - width;
                buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo);
            }
            break;
        }

        default:
        {
            LONG x, y;

            LOCK_X11
            for (y = 0; y < height; y++)
            {
                HIDDT_Pixel *p;

                p = buf;
                for (x = 0; x < width; x++)
                {
                    *p++ = XGetPixel(image, x, y);
                }
                buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo);
            }
            UNLOCK_X11
            break;
        }

        } /* switch (image->bits_per_pixel) */

        break;

    default:
    {

        OOP_Object *srcpf, *dstpf, *gfxhidd;
        APTR srcPixels = image->data, dstBuf = buf;

        //bug("DEFAULT PIXEL CONVERSION\n");

        OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, (IPTR *) &gfxhidd);
        dstpf = HIDD_Gfx_GetPixFmt(gfxhidd, msg->pixFmt);

        OOP_GetAttr(bm, aHidd_BitMap_PixFmt, (IPTR *) &srcpf);

        //bug("CALLING ConvertPixels()\n");

        HIDD_BM_ConvertPixels(bm, &srcPixels, (HIDDT_PixelFormat *) srcpf, image->bytes_per_line, &dstBuf,
                (HIDDT_PixelFormat *) dstpf, msg->modulo, width, height, NULL /* We have no CLUT */
                );

        //bug("CONVERTPIXELS DONE\n");

        buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo * height);
        break;
    }

    } /* switch (msg->pixFmt) */

    return buf;
}

/****************************************************************************************/

#define ABS(a) ((a) < 0 ? -(a) : a)

/****************************************************************************************/

static inline UBYTE pix_to_lut(HIDDT_Pixel pixel, HIDDT_PixelLUT *plut, HIDDT_PixelFormat *pf)
{
    HIDDT_ColComp red, green, blue;
    ULONG i, best_match = 0;
    ULONG diff, lowest_diff = 0xFFFFFFFF;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    red = RED_COMP(pixel, pf);
    green = GREEN_COMP(pixel, pf);
    blue = BLUE_COMP(pixel, pf);

    for (i = 0; i < plut->entries; i++)
    {
        register HIDDT_Pixel cur_lut = plut->pixels[i];

        if (pixel == cur_lut)
            return i; /* Exact match found */

        /* How well does these pixels match ? */
        diff = ABS(red - RED_COMP(cur_lut, pf)) + ABS(green - GREEN_COMP(cur_lut, pf))
                + ABS(blue - BLUE_COMP(cur_lut, pf));

        if (diff < lowest_diff)
        {
            best_match = i;
            lowest_diff = diff;
        }

    }

    return best_match;
}

/****************************************************************************************/

static UBYTE *ximage_to_buf_lut(OOP_Class *cl, OOP_Object *bm, UBYTE *buf, XImage *image, ULONG width, ULONG height,
        ULONG depth, struct pHidd_BitMap_GetImageLUT *msg)
{
    /* This one is trickier, as we have to reverse-lookup the lut.
     This costs CPU ! Maybe one could do some kind of caching here ?
     Ie. one stores the most often used RGB combinations
     in a trie and looks up this first to see if whe can find an exact match
     */

    HIDDT_PixelFormat *pf = BM_PIXFMT(bm);
    UBYTE *pixarray = msg->pixels;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    if (image->bits_per_pixel == 16)
    {
        UWORD *imdata = (UWORD *) image->data;
        LONG x, y;

        for (y = 0; y < height; y++)
        {
            UBYTE *buf = pixarray;

            for (x = 0; x < width; x++)
            {
                *buf++ = pix_to_lut((HIDDT_Pixel) *imdata, msg->pixlut, pf);

                imdata++;
            }
            imdata += ((image->bytes_per_line / 2) - width); /*sg*/

            pixarray += msg->modulo;

        }
    }
    else
    {
        LONG x, y;

        LOCK_X11
        for (y = 0; y < height; y++)
        {
            UBYTE *buf = pixarray;
            for (x = 0; x < width; x++)
            {
                *buf++ = pix_to_lut((HIDDT_Pixel) XGetPixel(image, x, y), msg->pixlut, pf);
                ;
            }
            pixarray += msg->modulo;

        }
        UNLOCK_X11

    }

    return pixarray;

}

/****************************************************************************************/

#if USE_XSHM

/****************************************************************************************/

static void getimage_xshm(OOP_Class *cl, OOP_Object *o, LONG x, LONG y,
        ULONG width, ULONG height, APTR pixarray,
        APTR (*fromimage_func)(), APTR fromimage_data)
{
    struct bitmap_data *data;
    XImage *image;
    IPTR depth;
    ULONG lines_to_copy;
    LONG ysize;
    LONG current_y;
    LONG maxlines;
    OOP_Object *pf;
    Pixmap temp_pixmap = 0;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    ASSERT(width > 0 && height > 0);

    data = OOP_INST_DATA(cl, o);

    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

    LOCK_X11
    image = create_xshm_ximage(data->display,
            DefaultVisual(data->display, data->screen),
            depth,
            ZPixmap,
            width,
            height,
            XSD(cl)->xshm_info);
    UNLOCK_X11

    if (!image)
        return;

    ASSERT(image->bytes_per_line > 0);

    /* Calculate how many scanline can be stored in the buffer */
    maxlines = XSHM_MEMSIZE / image->bytes_per_line;

    if (0 == maxlines)
    {
        bug("ALERT !!! NOT ENOUGH MEMORY TO READ A COMPLETE SCANLINE\n");
        bug("THROUGH XSHM IN X11GF X HIDD !!!\n");
        Alert(AT_DeadEnd);
    }

    current_y = 0;
    ysize = image->height;

    ObtainSemaphore(&XSD(cl)->shm_sema);

    LOCK_X11

    while (ysize)
    {
        /* Get some more pixels from the Ximage */

        lines_to_copy = MIN(maxlines, ysize);

        ysize -= lines_to_copy;
        image->height = lines_to_copy;

        if (!temp_pixmap)
        {
            if (!(get_xshm_ximage(data->display, DRAWABLE(data), image,
                                    x, y + current_y)))
            {
                /* XGetImage fails if done on a part of a X window which is off
                 screen (OTOH, it's no problem if it is hidden by another window).
                 If get_xshm_image() failed, we assume that this has happened and
                 thefore copy the area to a temp pixmap, and then get the ximage
                 from there. */

                temp_pixmap = XCALL(XCreatePixmap, data->display, DRAWABLE(data),
                        width, height - current_y, DefaultDepth(data->display, data->screen));

                if (temp_pixmap)
                {
                    XCALL(XSetFunction, data->display, data->gc, GXcopy);

                    XCALL(XCopyArea, data->display, DRAWABLE(data), temp_pixmap, data->gc,
                            x, y + current_y, width, height - current_y, 0, 0);

                    XCALL(XSetFunction, data->display, data->gc, GC_DRMD(data->gc));

                    x = 0; y = 0; current_y = 0;
                }
            }
        }

        if (temp_pixmap)
        {
            get_xshm_ximage(data->display, temp_pixmap, image,
                    x, y + current_y);
        }

        current_y += lines_to_copy;

        pixarray = fromimage_func(cl, o, pixarray, image, image->width,
                lines_to_copy, depth, fromimage_data);

    } /* while (pixels left to copy) */

    if (temp_pixmap)
    {
        XCALL(XFreePixmap,data->display, temp_pixmap);
    }

    UNLOCK_X11

    ReleaseSemaphore(&XSD(cl)->shm_sema);

    LOCK_X11
    destroy_xshm_ximage(image);
    UNLOCK_X11

    return;

}

/****************************************************************************************/

#endif

/****************************************************************************************/

static void getimage_xlib(OOP_Class *cl, OOP_Object *o, LONG x, LONG y, ULONG width, ULONG height, APTR pixels,
        APTR(*fromimage_func)(), APTR fromimage_data)
{
    struct bitmap_data *data;
    XImage *image;
    ULONG *pixarray = (ULONG *) pixels;
    OOP_Object *pf;
    IPTR depth;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    data = OOP_INST_DATA(cl, o);

    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *) &pf);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

    LOCK_X11
    image = XCALL(XGetImage, data->display, DRAWABLE(data), x, y,
            width, height, AllPlanes, ZPixmap);
    UNLOCK_X11

    if (!image)
        return;

#if DO_ENDIAN_FIX
    if (NEEDS_ENDIAN_FIX(image))
    {
        SwapImageEndianess(image);

        LOCK_X11
        XCALL(XInitImage, image);
        UNLOCK_X11
    }
#endif

    fromimage_func(cl, o, pixarray, image, width, height, depth, fromimage_data);

    LOCK_X11
    XDestroyImage(image);
    UNLOCK_X11

    return;
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__GetImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    ASSERT(msg->width > 0 && msg->height > 0);

#if USE_XSHM
    if (XSD(cl)->use_xshm)
    {
        getimage_xshm(cl, o, msg->x, msg->y, msg->width, msg->height,
                msg->pixels, (APTR (*)())ximage_to_buf, msg);
    }
    else
#endif
    {
        getimage_xlib(cl, o, msg->x, msg->y, msg->width, msg->height, msg->pixels, (APTR(*)()) ximage_to_buf, msg);
    }
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__GetImageLUT)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    ASSERT(msg->width != 0 && msg->height != 0);
#if USE_XSHM
    if (XSD(cl)->use_xshm)
    {
        getimage_xshm(cl, o, msg->x, msg->y, msg->width, msg->height,
                msg->pixels, (APTR (*)())ximage_to_buf_lut, msg);
    }
    else
#endif
    {
        getimage_xlib(cl, o, msg->x, msg->y, msg->width, msg->height, msg->pixels, (APTR(*)()) ximage_to_buf_lut, msg);
    }
}

/****************************************************************************************/

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

static ULONG *buf_to_ximage(OOP_Class *cl, OOP_Object *bm, HIDDT_Pixel *buf, XImage *image, ULONG width, ULONG height,
        ULONG depth, struct pHidd_BitMap_PutImage *msg)
{
    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    switch (msg->pixFmt)
    {
    case vHidd_StdPixFmt_Native:
    {
        UBYTE *imdata = image->data;
        LONG y;

        for (y = 0; y < height; y++)
        {
            memcpy(imdata, buf, msg->width * image->bits_per_pixel / 8);

            imdata += image->bytes_per_line;
            buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo);
        }
    }
        break;

    case vHidd_StdPixFmt_Native32:
        switch (image->bits_per_pixel)
        {
        case 8:
        {
            UBYTE *imdata = (UBYTE *) image->data;
            LONG x, y;

            for (y = 0; y < height; y++)
            {
                HIDDT_Pixel *p = buf;

                for (x = 0; x < width; x++)
                {
                    *imdata++ = (UBYTE) *p++;
                }
                imdata += image->bytes_per_line - width;
                buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo);
            }
            break;
        }

        case 16:
        {
            UWORD *imdata = (UWORD *) image->data;
            LONG x, y;

            for (y = 0; y < height; y++)
            {
                HIDDT_Pixel *p = buf;

                for (x = 0; x < width; x++)
                {
                    *imdata++ = (UWORD) *p++;
                }
                imdata += image->bytes_per_line / 2 - width;
                buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo);
            }
            break;
        }

        case 24:
        {
            HIDDT_PixelFormat *pf;
            UBYTE *imdata = image->data;
            LONG x, y;

            pf = BM_PIXFMT(bm);

            for (y = 0; y < height; y++)
            {

                HIDDT_Pixel *p = buf;

                for (x = 0; x < width; x++)
                {
                    register HIDDT_Pixel pix;

                    pix = *p++;
#if (AROS_BIG_ENDIAN == 1)
                    *imdata ++ = pix >> 16;
                    *imdata ++ = (pix & pf->green_mask) >> 8;
                    *imdata ++ = (pix & pf->blue_mask);
#else
                    *imdata++ = (pix & pf->blue_mask);
                    *imdata++ = (pix & pf->green_mask) >> 8;
                    *imdata++ = pix >> 16;
#endif
                }
                imdata += image->bytes_per_line - width * 3;
                buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo);
            }
            break;
        }

        case 32:
        {
            ULONG *imdata = (ULONG *) image->data;
            LONG x, y;

            for (y = 0; y < height; y++)
            {
                HIDDT_Pixel *p = buf;

                for (x = 0; x < width; x++)
                {
                    *imdata++ = (ULONG) *p++;
                }
                imdata += image->bytes_per_line / 4 - width;
                buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo);
            }
            break;
        }

        default:
        {
            LONG x, y;

            LOCK_X11
            for (y = 0; y < height; y++)
            {
                HIDDT_Pixel *p;

                p = buf;
                for (x = 0; x < width; x++)
                {
                    XPutPixel(image, x, y, *p++);
                }
                buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo);
            }
            UNLOCK_X11
            break;
        }

        } /* switch (image->bits_per_pixel) */

        break;

    default:
    {
        OOP_Object *srcpf, *dstpf, *gfxhidd;
        APTR srcPixels = buf, dstBuf = image->data;

        //bug("DEFAULT PIXEL CONVERSION\n");

        OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, (IPTR *) &gfxhidd);
        srcpf = HIDD_Gfx_GetPixFmt(gfxhidd, msg->pixFmt);

        OOP_GetAttr(bm, aHidd_BitMap_PixFmt, (IPTR *) &dstpf);

        //bug("CALLING ConvertPixels()\n");

        HIDD_BM_ConvertPixels(bm, &srcPixels, (HIDDT_PixelFormat *) srcpf, msg->modulo, &dstBuf,
                (HIDDT_PixelFormat *) dstpf, image->bytes_per_line, width, height, NULL); /* We have no CLUT */

        //bug("CONVERTPIXELS DONE\n");

        buf = (HIDDT_Pixel *) ((UBYTE *) buf + msg->modulo * height);
        break;
    }

    } /* switch (msg->pixFmt) */

    return buf;
}

/****************************************************************************************/

static UBYTE *buf_to_ximage_lut(OOP_Class *cl, OOP_Object *bm, UBYTE *pixarray, XImage *image, ULONG width,
        ULONG height, ULONG depth, struct pHidd_BitMap_PutImageLUT *msg)
{
    HIDDT_Pixel *lut = msg->pixlut->pixels;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    switch (image->bits_per_pixel)
    {
    case 8:
    {
        UBYTE *imdata = (UBYTE *) image->data;
        LONG x, y;

        for (y = 0; y < height; y++)
        {
            UBYTE *buf = pixarray;

            for (x = 0; x < width; x++)
            {
                *imdata++ = (UBYTE) lut[*buf++];
            }
            pixarray += msg->modulo;
            imdata += (image->bytes_per_line - width); /*sg*/
        }
        break;
    }

    case 16:
    {
        UWORD *imdata = (UWORD *) image->data;
        LONG x, y;

        for (y = 0; y < height; y++)
        {
            UBYTE *buf = pixarray;

            for (x = 0; x < width; x++)
            {
                *imdata++ = (UWORD) lut[*buf++];
            }
            pixarray += msg->modulo;
            imdata += ((image->bytes_per_line / 2) - width); /*sg*/
        }
        break;
    }

    case 32:
    {
        ULONG *imdata = (ULONG *) image->data;
        LONG x, y;

        for (y = 0; y < height; y++)
        {
            UBYTE *buf = pixarray;

            for (x = 0; x < width; x++)
            {
                *imdata++ = (ULONG) lut[*buf++];

            }
            pixarray += msg->modulo;
            imdata += ((image->bytes_per_line / 4) - width); /*sg*/
        }
        break;
    }

    default:
    {
        LONG x, y;

        for (y = 0; y < height; y++)
        {
            UBYTE *buf = pixarray;

            for (x = 0; x < width; x++)
            {
                XPutPixel(image, x, y, lut[*buf++]);
            }

            pixarray += msg->modulo;

        }
        break;

    }

    } /* switch(image->bits_per_pixel) */

    return pixarray;
}

/****************************************************************************************/

#if USE_XSHM

/****************************************************************************************/

static void putimage_xshm(OOP_Class *cl, OOP_Object *o, OOP_Object *gc,
        LONG x, LONG y, ULONG width, ULONG height,
        APTR pixarray, APTR (*toimage_func)(), APTR toimage_data)
{

    struct bitmap_data *data;
    XImage *image;
    IPTR depth;
    ULONG lines_to_copy;
    LONG ysize;
    LONG current_y;
    LONG maxlines;
    OOP_Object *pf;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    data = OOP_INST_DATA(cl, o);

    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

    LOCK_X11
    image = create_xshm_ximage(data->display,
            DefaultVisual(data->display, data->screen),
            depth,
            ZPixmap,
            width,
            height,
            XSD(cl)->xshm_info);
    UNLOCK_X11

    if (!image)
        return;

    /* Calculate how many scanline can be stored in the buffer */
    maxlines = XSHM_MEMSIZE / image->bytes_per_line;

    if (0 == maxlines)
    {
        bug("ALERT !!! NOT ENOUGH MEMORY TO WRITE A COMPLETE SCANLINE\n");
        bug("THROUGH XSHM IN X11GF X HIDD !!!\n");
        Alert(AT_DeadEnd);
    }

    current_y = 0;
    ysize = image->height;

    ObtainSemaphore(&XSD(cl)->shm_sema);

    while (ysize)
    {
        /* Get some more pixels from the HIDD */

        lines_to_copy = MIN(maxlines, ysize);

        ysize -= lines_to_copy;
        image->height = lines_to_copy;

        pixarray = toimage_func(cl, o, pixarray, image, image->width,
                lines_to_copy, depth, toimage_data);

        LOCK_X11
        XCALL(XSetFunction, data->display, data->gc, GC_DRMD(gc));

        put_xshm_ximage(data->display,
                DRAWABLE(data),
                data->gc,
                image,
                0, 0,
                x, y + current_y,
                image->width, lines_to_copy,
                FALSE);

        UNLOCK_X11

        current_y += lines_to_copy;

    } /* while (pixels left to copy) */

    ReleaseSemaphore(&XSD(cl)->shm_sema);

    LOCK_X11
    destroy_xshm_ximage(image);
    UNLOCK_X11

    return;

}

/****************************************************************************************/

#endif

/****************************************************************************************/

static void putimage_xlib(OOP_Class *cl, OOP_Object *o, OOP_Object *gc, LONG x, LONG y, ULONG width, ULONG height,
        APTR pixarray, APTR(*toimage_func)(), APTR toimage_data)
{

    struct bitmap_data *data;
    XImage *image;
    ULONG bperline;
    IPTR depth;
    OOP_Object *pf;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    data = OOP_INST_DATA(cl, o);

    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *) &pf);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

    LOCK_X11
    image = XCALL(XCreateImage, data->display,
            DefaultVisual(data->display, data->screen),
            depth,
            ZPixmap,
            0,
            NULL,
            width,
            height,
            32,
            0);
    UNLOCK_X11

    if (!image)
        return;

#if DO_ENDIAN_FIX
    if (NEEDS_ENDIAN_FIX(image))
    {
        image->byte_order = AROS_BYTEORDER;

        LOCK_X11
        XCALL(XInitImage, image);
        UNLOCK_X11
    }
#endif

    bperline = image->bytes_per_line;
    image->data = (char *) AllocVec((size_t) height * bperline, MEMF_PUBLIC);

    if (!image->data)
    {
        LOCK_X11
        XCALL(XFree, image);
        UNLOCK_X11

        return;
    }

    toimage_func(cl, o, pixarray, image, width, height, depth, toimage_data);

    LOCK_X11
    XCALL(XSetFunction, data->display, data->gc, GC_DRMD(gc));
    XCALL( XPutImage, data->display, DRAWABLE(data), data->gc, image, 0, 0, x, y, width, height);
    UNLOCK_X11

    FreeVec(image->data);

    LOCK_X11
    XCALL(XFree, image);
    UNLOCK_X11

}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__PutImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    D(bug("[X11Bm] %s(pa=%p, x=%d, y=%d, w=%d, h=%d)\n", __PRETTY_FUNCTION__, msg->pixels, msg->x, msg->y, msg->width,
                    msg->height));

#if USE_XSHM
    if (XSD(cl)->use_xshm)
    {
        putimage_xshm(cl, o, msg->gc, msg->x, msg->y,
                msg->width, msg->height, msg->pixels,
                (APTR (*)()) buf_to_ximage, msg);
    }
    else
#endif
    {
        putimage_xlib(cl, o, msg->gc, msg->x, msg->y, msg->width, msg->height, msg->pixels, (APTR(*)()) buf_to_ximage, msg);
    }
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__PutImageLUT)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    D(bug("[X11Bm] %s(pa=%p, x=%d, y=%d, w=%d, h=%d)\n", __PRETTY_FUNCTION__, msg->pixels, msg->x, msg->y, msg->width,
                    msg->height));

#if USE_XSHM
    if (XSD(cl)->use_xshm)
    {
        putimage_xshm(cl, o, msg->gc, msg->x, msg->y,
                msg->width, msg->height, msg->pixels,
                (APTR (*)())buf_to_ximage_lut, msg);
    }
    else
#endif
    {
        putimage_xlib(cl, o, msg->gc, msg->x, msg->y, msg->width, msg->height, msg->pixels, (APTR(*)()) buf_to_ximage_lut, msg);
    }
}

/****************************************************************************************/

VOID MNAME(Root__Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    if (IS_X11BM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_X11BitMap_Drawable:
            *msg->storage = (IPTR) DRAWABLE(data);
            return;

        case aoHidd_X11BitMap_GC:
            *msg->storage = (IPTR) data->gc;
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

/****************************************************************************************/

BOOL X11BM__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

#if ADJUST_XWIN_SIZE
    /* This provides support for framebuffer display mode switching */
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    if (data->flags & BMDF_FRAMEBUFFER)
    {
        struct TagItem *tag = FindTagItem(aHidd_BitMap_ModeID, msg->attrList);

        if (tag)
        {
            if (!X11BM_SetMode(data, tag->ti_Data, XSD(cl)))
            {
                /* Bail out if error happened, see aoHidd_BitMap_ModeID documentation */
                return FALSE;
            }
        }
    }
#endif
    return OOP_DoSuperMethod(cl, o, &msg->mID);
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__DrawLine)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawLine *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    OOP_Object *gc = msg->gc;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    if (GC_LINEPAT(gc) != (UWORD) ~0)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

        return;
    }

    LOCK_X11

    if (GC_DOCLIP(gc))
    {
        XRectangle cr;

        cr.x = GC_CLIPX1(gc);
        cr.y = GC_CLIPY1(gc);
        cr.width = GC_CLIPX2(gc) - cr.x + 1;
        cr.height = GC_CLIPY2(gc) - cr.y + 1;

        XCALL(XSetClipRectangles, data->display, data->gc, 0, 0, &cr, 1, Unsorted);
    }

    XCALL(XSetForeground, data->display, data->gc, GC_FG(gc));
    XCALL(XSetFunction, data->display, data->gc, GC_DRMD(gc));

    XCALL( XDrawLine, data->display, DRAWABLE(data), data->gc, msg->x1, msg->y1, msg->x2, msg->y2);

    if (GC_DOCLIP(gc))
    {
        XCALL(XSetClipMask, data->display, data->gc, None);
    }

    UNLOCK_X11
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__DrawEllipse)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawEllipse *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    OOP_Object *gc = msg->gc;

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    LOCK_X11

    if (GC_DOCLIP(gc))
    {
        XRectangle cr;

        /* bug("X11::Drawllipse: clip %d %d %d %d\n"
         , GC_CLIPX1(gc), GC_CLIPY1(gc), GC_CLIPX2(gc), GC_CLIPY2(gc));
         */

        cr.x = GC_CLIPX1(gc);
        cr.y = GC_CLIPY1(gc);
        cr.width = GC_CLIPX2(gc) - cr.x + 1;
        cr.height = GC_CLIPY2(gc) - cr.y + 1;

        XCALL(XSetClipRectangles, data->display, data->gc, 0, 0, &cr, 1, Unsorted);
    }

    XCALL(XSetForeground, data->display, data->gc, GC_FG(gc));
    XCALL(XSetFunction, data->display, data->gc, GC_DRMD(gc));

    /* bug("X11::Drawllipse: coord %d %d %d %d\n"
     , msg->x, msg->y, msg->rx, msg->ry);

     */

    XCALL(XDrawArc, data->display, DRAWABLE(data), data->gc, msg->x - msg->rx, msg->y - msg->ry, msg->rx * 2, msg->ry * 2, 0, 360 * 64);

    if (GC_DOCLIP(gc))
    {
        XCALL(XSetClipMask, data->display, data->gc, None);
    }

    UNLOCK_X11
}

/****************************************************************************************/

VOID MNAME(Hidd_BitMap__UpdateRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    D(bug("[X11Bm] %s()\n", __PRETTY_FUNCTION__));

    LOCK_X11

    if (data->flags & BMDF_FRAMEBUFFER)
        X11BM_ExposeFB(data, msg->x, msg->y, msg->width, msg->height);

    XCALL(XFlush, data->display);

    UNLOCK_X11
}

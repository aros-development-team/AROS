#include <graphics/renderfunc.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/* Private lowlevel functions needed by cybergraphics.library */

AROS_LH9(LONG, WritePixels8,
	 AROS_LHA(struct RastPort, *rp, A0),
	 AROS_LHA(UBYTE, *array, A1),
	 AROS_LHA(ULONG, modulo, D0),
	 AROS_LHA(LONG, xstart, D1),
	 AROS_LHA(LONG, ystart, D2),
	 AROS_LHA(LONG, xstop, D3),
	 AROS_LHA(LONG, ystop, D4),
	 AROS_LHA(HIDDT_PixelLUT *, pixlut, A2),
	 AROS_LHA(BOOL, do_update, D5),
	 struct GfxBase *, GfxBase, 182, Graphics)
{
    AROS_LIBFUNC_INIT

    return write_pixels_8(rp, array, modulo, xstart, ystart, xstop, ystop, pixlut, do_update, GfxBase);

    AROS_LIBFUNC_EXIT
}

AROS_LH8(LONG, FillRectPenDrMd,
	 AROS_LHA(struct RastPort, *rp, A0),
	 AROS_LHA(LONG, x1, D0),
	 AROS_LHA(LONG, y1, D1),
	 AROS_LHA(LONG, x2, D2),
	 AROS_LHA(LONG, y2, D3),
    	 AROS_LHA(HIDDT_Pixel, pix, D4),
	 AROS_LHA(HIDDT_DrawMode, drmd, D5),
	 AROS_LHA(BOOL, do_update, D6),
	 struct GfxBase *, GfxBase, 183, Graphics)
{
    AROS_LIBFUNC_INIT

    return fillrect_pendrmd(rp, x1, y1, x2, y2, pix, drmd, do_update, GfxBase);

    AROS_LIBFUNC_EXIT
}

AROS_LH6(ULONG, DoRenderFunc,
	 AROS_LHA(struct RastPort, *rp, A0),
	 AROS_LHA(Point, *src, A1),
	 AROS_LHA(struct Rectangle, *rr, A2),
	 AROS_LHA(RENDERFUNC, render_func, A3),
	 AROS_LHA(APTR, funcdata, A4),
	 AROS_LHA(BOOL, do_update, D0),
	 struct GfxBase *, GfxBase, 184, Graphics)
{
    AROS_LIBFUNC_INIT

    ULONG res;

    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
        return -1;

    res = do_render_func(rp, src, rr, render_func, funcdata, do_update, FALSE, GfxBase);

    RELEASE_DRIVERDATA(rp, GfxBase);
    return res;

    AROS_LIBFUNC_EXIT
}

AROS_LH6(LONG, DoPixelFunc,
	 AROS_LHA(struct RastPort, *rp, A0),
	 AROS_LHA(LONG, x, D0),
	 AROS_LHA(LONG, y, D1),
    	 AROS_LHA(PIXELFUNC, render_func, A1),
	 AROS_LHA(APTR, funcdata, A2),
	 AROS_LHA(BOOL, do_update, D2),
	 struct GfxBase *, GfxBase, 185, Graphics)
{
    AROS_LIBFUNC_INIT

    LONG res;

    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
        return -1;

    res = do_pixel_func(rp, x, y, render_func, funcdata, do_update, GfxBase);

    RELEASE_DRIVERDATA(rp, GfxBase);
    return res;

    AROS_LIBFUNC_EXIT
}

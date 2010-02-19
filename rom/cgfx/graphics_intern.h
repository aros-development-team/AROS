/*
 * These functions are statically linked in from graphics.library.
 * We may completely separate cybergraphics.library from graphics.library
 * if we turn them into private LVOs.
 */

LONG write_pixels_8(struct RastPort *rp, UBYTE *array, ULONG modulo,
    	    	    LONG xstart, LONG ystart, LONG xstop, LONG ystop,
		    HIDDT_PixelLUT *pixlut, BOOL do_update, struct GfxBase *GfxBase);

OOP_Object *GetDriverGC(struct RastPort *rp, struct GfxBase *GfxBase);

void ReleaseDriverData(struct RastPort *rp, struct GfxBase *GfxBase);

LONG fillrect_pendrmd(struct RastPort *tp, LONG x1, LONG y1, LONG x2, LONG y2,
    	    	      HIDDT_Pixel pix, HIDDT_DrawMode drmd, BOOL do_update, struct GfxBase *GfxBase);

ULONG DoPixelFunc(struct RastPort *rp, LONG x, LONG y,
    	    	    LONG (*render_func)(APTR, OOP_Object *, OOP_Object *, LONG, LONG, struct GfxBase *),
		    APTR funcdata, BOOL do_update, struct GfxBase *GfxBase);

ULONG DoRenderFunc(struct RastPort *rp, Point *src, struct Rectangle *rr,
	    	     ULONG (*render_func)(APTR, LONG, LONG, OOP_Object *, OOP_Object *, LONG, LONG, LONG, LONG, struct GfxBase *),
		     APTR funcdata, BOOL do_update, struct GfxBase *GfxBase);

#define WritePixels8(rp, array, modulo, xstart, ystart, xstop, ystop, pixlut, do_update) \
	write_pixels_8(rp, array, modulo, xstart, ystart, xstop, ystop, pixlut, do_update, GfxBase)

#define GetDriverGC(rp) GetDriverGC(rp, GfxBase)

#define ReleaseDriverData(rp) ReleaseDriverData(rp, GfxBase)

#define FillRectPenDrMd(rp, x1, y1, x2, y2, pix, drmd, update) fillrect_pendrmd(rp, x1, y1, x2, y2, pix, drmd, update, GfxBase)

#define DoPixelFunc(rp, x, y, func, data, update) DoPixelFunc(rp, x, y, func, data, update, GfxBase)

#define DoRenderFunc(rp, src, rr, func, data, update) DoRenderFunc(rp, src, rr, func, data, update, GfxBase)

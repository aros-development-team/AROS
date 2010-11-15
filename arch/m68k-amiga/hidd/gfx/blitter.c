
#include <proto/exec.h>
#include <proto/graphics.h>

#include <exec/libraries.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hidd/graphics.h>

#include "amigavideogfx.h"
#include "amigavideobitmap.h"
#include "blitter.h"

#define DEBUG 1
#include <aros/debug.h>

static void waitblitter(void)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    for(;;) {
    	if (!(custom->dmaconr & 0x4000))
    	    break;
    }
}

static const UWORD leftmask[] = {
    0xffff, 0x7fff, 0x3fff, 0x1fff,
    0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f,
    0x000f, 0x0007, 0x0003, 0x0001
};
static const UWORD rightmask[] = {
    0x8000, 0xc000, 0xe000, 0xf000,
    0xf800, 0xfc00, 0xfe00, 0xff00,
    0xff80, 0xffc0, 0xffe0, 0xfff0,
    0xfff8, 0xfffc, 0xfffe, 0xffff
};

static UBYTE copy_minterm[] = { 0x00, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00 };
	
BOOL blit_copybox(struct amigavideo_staticdata *data, struct planarbm_data *srcbm, struct planarbm_data *dstbm,
    WORD srcx, WORD srcy, WORD w, WORD h, WORD dstx, WORD dsty, HIDDT_DrawMode mode)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    ULONG srcoffset, dstoffset;
    BYTE shift, i;
    WORD srcwidth, dstwidth, width;
    WORD srcx2, dstx2;
    UWORD shifta, shiftb;
    UWORD afwm, alwm;
    BOOL reverse = FALSE;

    if (USE_BLITTER == 0)
    	return FALSE;

    srcx2 = srcx + w - 1;
    dstx2 = dstx + w - 1;
    if (mode == vHidd_GC_DrawMode_Clear || mode == vHidd_GC_DrawMode_Set)
    	return blit_fillrect(data, dstbm, dstx, dsty, dstx2, dsty + h - 1, 0, mode);

    if (copy_minterm[mode] == 0)
    	return FALSE;

    if (srcbm == dstbm) // we can't handle overlapping yet
    	return FALSE;

    D(bug("%x %x %dx%d %dx%d %dx%d %d\n", srcbm, dstbm, srcx, srcy, dstx, dsty, w, h, mode));
 
    srcoffset = srcbm->bytesperrow * srcy + (srcx / 16) * 2;
    dstoffset = dstbm->bytesperrow * dsty + (dstx / 16) * 2;
    shift = (dstx & 15) - (srcx & 15);

    srcwidth = (((srcx2 + 15) & ~15) - (srcx & ~15)) / 16;
    dstwidth = (((dstx2 + 15) & ~15) - (dstx & ~15)) / 16;
    if (srcwidth == 0)
    	srcwidth = 1;
    if (dstwidth == 0)
    	dstwidth = 1;

    if (shift < 0) {
   	shift = -shift;
    	reverse = TRUE;
     	if (dstwidth >= srcwidth) {
    	    width = dstwidth;
    	    shifta = 0;
    	    shiftb = shift << 12;
    	    alwm = leftmask[dstx & 15];
    	    afwm = rightmask[dstx2 & 15];
    	} else {
    	   width = srcwidth;
    	   shifta = shift << 12;
    	   shiftb = shift << 12;
     	   alwm = leftmask[srcx & 15];
    	   afwm = rightmask[srcx2 & 15];
    	}
    	srcoffset += srcbm->bytesperrow * (h - 1) + width * 2 - 2;
    	dstoffset += dstbm->bytesperrow * (h - 1) + width * 2 - 2;
    } else {
     	if (dstwidth >= srcwidth) {
    	    width = dstwidth;
    	    shifta = 0;
    	    shiftb = shift << 12;
    	    afwm = leftmask[dstx & 15];
    	    alwm = rightmask[dstx2 & 15];
    	} else {
    	   width = srcwidth;
    	   shifta = shift << 12;
    	   shiftb = shift << 12;
     	   afwm = leftmask[srcx & 15];
    	   alwm = rightmask[srcx2 & 15];
    	}
    }

    D(bug("shift=%d rev=%d sw=%d dw=%d %04x %04x\n", shift, reverse, srcwidth, dstwidth, afwm, alwm));

    OwnBlitter();
    waitblitter();

    custom->bltafwm = afwm;
    custom->bltalwm = alwm;
    custom->bltbmod = srcbm->bytesperrow - width * 2;
    custom->bltcmod = dstbm->bytesperrow - width * 2;
    custom->bltdmod = dstbm->bytesperrow - width * 2;
    custom->bltcon1 = (reverse ? 0x0002 : 0x0000) | shiftb;
    custom->bltcon0 = 0x0700 | copy_minterm[mode] | shifta;
    custom->bltadat = 0xffff;
    
    for (i = 0; i < srcbm->depth && i < dstbm->depth; i++) {
    	waitblitter();
     	custom->bltbpt = (APTR)(srcbm->planes[i] + srcoffset);
    	custom->bltcpt = (APTR)(dstbm->planes[i] + dstoffset);
    	custom->bltdpt = (APTR)(dstbm->planes[i] + dstoffset);
    	custom->bltsize = (h << 6) | width;
    }

    waitblitter();
    DisownBlitter();

    return TRUE;
}

static UBYTE fill_minterm[] = { 0xca, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x00, 0x00, 0xca };
// copy: /AC + AB = 0xCA
// invert: /AC + A/C = 0x5A
// A-DAT = edge masking
// B-DAT = pixel value (ones or zeros)
// C     = source
// D     = destination

BOOL blit_fillrect(struct amigavideo_staticdata *data, struct planarbm_data *bm, WORD x1,WORD y1,WORD x2,WORD y2, HIDDT_Pixel pixel,HIDDT_DrawMode mode)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    ULONG offset;
    WORD width, height;
    UBYTE i;
    struct Library *GfxBase = data->GfxBase;
    
    //D(bug("fillrect(%dx%d,%dx%d,%d,%d)\n", x1, y1, x2, y2, pixel, mode));

    if (fill_minterm[mode] == 0 || USE_BLITTER == 0)
    	return FALSE;

    offset = bm->bytesperrow * y1 + (x1 / 16) * 2;
    width = ((x2 + 15) / 16) - (x1 / 16);
    if (width == 0)
    	width = 1;
    height = y2 - y1 + 1;

    OwnBlitter();
    waitblitter();

    custom->bltafwm = leftmask[x1 & 15];
    custom->bltalwm = rightmask[x2 & 15];
    custom->bltcmod = bm->bytesperrow - width * 2;
    custom->bltdmod = bm->bytesperrow - width * 2;
    custom->bltcon1 = 0x0000;
    custom->bltcon0 = 0x0300 | fill_minterm[mode];
    custom->bltadat = 0xffff;
    
    if (mode == vHidd_GC_DrawMode_Clear)
    	pixel = 0x00;
    else if (mode == vHidd_GC_DrawMode_Set)
    	pixel = 0xff;
    
    for (i = 0; i < bm->depth; i++) {
    	waitblitter();
    	custom->bltbdat = (pixel & 1) ? 0xffff : 0x0000;
    	custom->bltcpt = (APTR)(bm->planes[i] + offset);
    	custom->bltdpt = (APTR)(bm->planes[i] + offset);
    	custom->bltsize = (height << 6) | width;
    	pixel >>= 1;
    }

    waitblitter();
    DisownBlitter();

    return TRUE;
}

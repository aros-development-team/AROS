
#include <proto/exec.h>
#include <proto/graphics.h>

#include <exec/libraries.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hidd/graphics.h>

#include "amigavideogfx.h"
#include "amigavideobitmap.h"
#include "blitter.h"

#define DEBUG 0
#include <aros/debug.h>

static void startblitter(struct amigavideo_staticdata *data, UWORD w, UWORD h)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    if (data->ecs_agnus) {
    	custom->bltsizv = h;
    	custom->bltsizh = w;
    } else {
    	if (h > 1024 || w > 64)
    	    return;
    	custom->bltsize = (h << 6) | (w & 63);
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

static UBYTE copy_minterm[] = { 0xff, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff };
// A-DAT = edge masking
// B     = source
// C     = destination
// D     = destination
//  0: zero (->fillrect)
//  1: src AND dst 		/AC + ABC
//  2: src AND NOT dst 		/AC + AB/C
//  3: src 			/AC + AB
//  4: NOT src AND dst		/AC + A/B/C
//  5: dst (nop)
//  6: src XOR dst		/AC + A
//  7:
//  8:
//  9:
// 10: NOT dst (->fillrect)
// 11:
// 12:
// 13:
// 14:
// 15: one (->fillrect)

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
    UWORD bltcon0;
    BOOL reverse = FALSE;
    BOOL intersect = FALSE;

    if (USE_BLITTER == 0)
    	return FALSE;

    srcx2 = srcx + w - 1;
    dstx2 = dstx + w - 1;
    if (copy_minterm[mode] == 0xff)
    	return blit_fillrect(data, dstbm, dstx, dsty, dstx2, dsty + h - 1, 0, mode);

    if (copy_minterm[mode] == 0)
    	return FALSE;

    //D(bug("%x %x %dx%d %dx%d %dx%d %d\n", srcbm, dstbm, srcx, srcy, dstx, dsty, w, h, mode));

    if (srcbm == dstbm) {
	intersect = !(srcx > dstx2 || srcx2 < dstx || srcy > dsty + h - 1 || srcy + h - 1 < dsty);
    }
 
    srcoffset = srcbm->bytesperrow * srcy + (srcx / 16) * 2;
    dstoffset = dstbm->bytesperrow * dsty + (dstx / 16) * 2;
    shift = (dstx & 15) - (srcx & 15);

    if (intersect) {
   	// do we need >16 bit edge mask? (not handled yet)
    	if (dstoffset < srcoffset && shift < 0) // asc + desc
    	    return FALSE;
    	if (dstoffset > srcoffset && shift > 0) // desc + asc
    	    return FALSE;
    }

    if (shift < 0) {
    	reverse = TRUE;
    	shift = -shift;
    } else if (intersect && dstoffset > srcoffset) {
    	reverse = TRUE;
    }

    srcwidth = srcx2 / 16 - srcx / 16 + 1;
    dstwidth = dstx2 / 16 - dstx / 16 + 1;

    if (reverse) {
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
    	srcoffset += srcbm->bytesperrow * (h - 1) + (width - 1) * 2;
    	dstoffset += dstbm->bytesperrow * (h - 1) + (width - 1) * 2;
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

    //D(bug("shift=%d rev=%d sw=%d dw=%d %04x %04x\n", shift, reverse, srcwidth, dstwidth, afwm, alwm));

    OwnBlitter();
    WaitBlit();

    custom->bltafwm = afwm;
    custom->bltalwm = alwm;
    custom->bltbmod = srcbm->bytesperrow - width * 2;
    custom->bltcmod = dstbm->bytesperrow - width * 2;
    custom->bltdmod = dstbm->bytesperrow - width * 2;
    custom->bltcon1 = (reverse ? 0x0002 : 0x0000) | shiftb;
    bltcon0 = 0x0700 | copy_minterm[mode] | shifta;
    custom->bltadat = 0xffff;
    
    for (i = 0; i < dstbm->depth; i++) {
    	UWORD bltcon0b = bltcon0;
    	if (dstbm->planes[i] == (UBYTE*)0x00000000 || dstbm->planes[i] == (UBYTE*)0xffffffff)
  	    continue;
    	WaitBlit();
    	if (i >= srcbm->depth || srcbm->planes[i] == (UBYTE*)0x00000000) {
    	    bltcon0b &= ~0x0400;
    	    custom->bltbdat = 0x0000;
    	} else if (srcbm->planes[i] == (UBYTE*)0xffffffff) {
    	    bltcon0b &= ~0x0400;
    	    custom->bltbdat = 0xffff;
    	} else {
     	    custom->bltbpt = (APTR)(srcbm->planes[i] + srcoffset);
     	}
    	custom->bltcon0 = bltcon0b;
    	custom->bltcpt = (APTR)(dstbm->planes[i] + dstoffset);
    	custom->bltdpt = (APTR)(dstbm->planes[i] + dstoffset);
    	startblitter(data, width, h);
    }

    WaitBlit();
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
    width = x2 / 16 - x1 / 16 + 1;
    height = y2 - y1 + 1;

    OwnBlitter();
    WaitBlit();

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
    	if (bm->planes[i] == (UBYTE*)0x00000000 || bm->planes[i] == (UBYTE*)0xffffffff)
  	    continue;
    	WaitBlit();
    	custom->bltbdat = (pixel & 1) ? 0xffff : 0x0000;
    	custom->bltcpt = (APTR)(bm->planes[i] + offset);
    	custom->bltdpt = (APTR)(bm->planes[i] + offset);
    	startblitter(data, width, height);
    	pixel >>= 1;
    }

    WaitBlit();
    DisownBlitter();

    return TRUE;
}

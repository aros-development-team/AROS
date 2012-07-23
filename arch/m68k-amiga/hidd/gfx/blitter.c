
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

#define ABC    0x80
#define ABNC   0x40
#define ANBC   0x20
#define ANBNC  0x10
#define NABC   0x08
#define NABNC  0x04
#define NANBC  0x02
#define NANBNC 0x01

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

static const UBYTE copy_minterm[] = { 0xff, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff };
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
//  7: copy source and blit thru mask
//  8:
//  9:
// 10: NOT dst (->fillrect)
// 11:
// 12:
// 13:
// 14:
// 15: one (->fillrect)

BOOL blit_copybox(struct amigavideo_staticdata *data, struct amigabm_data *srcbm, struct amigabm_data *dstbm,
    WORD srcx, WORD srcy, WORD w, WORD h, WORD dstx, WORD dsty, HIDDT_DrawMode mode)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;
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

    D(bug("copybox(%x %x %dx%d %dx%d %dx%d %d)\n", srcbm, dstbm, srcx, srcy, dstx, dsty, w, h, mode));

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
    srcx &= 15;
    dstx &= 15;
    dstx2 &= 15;
    srcx2 &= 15;

    if (reverse) {
     	if (dstwidth >= srcwidth) {
    	    width = dstwidth;
    	    shifta = 0;
    	    shiftb = shift << 12;
    	    alwm = leftmask[dstx];
    	    afwm = rightmask[dstx2];
    	} else {
    	   width = srcwidth;
    	   shifta = shift << 12;
    	   shiftb = shift << 12;
     	   alwm = leftmask[srcx];
    	   afwm = rightmask[srcx2];
    	}
    	srcoffset += srcbm->bytesperrow * (h - 1) + (width - 1) * 2;
    	dstoffset += dstbm->bytesperrow * (h - 1) + (width - 1) * 2;
    } else {
     	if (dstwidth >= srcwidth) {
    	    width = dstwidth;
    	    shifta = 0;
    	    shiftb = shift << 12;
    	    afwm = leftmask[dstx];
    	    alwm = rightmask[dstx2];
    	} else {
    	   width = srcwidth;
    	   shifta = shift << 12;
    	   shiftb = shift << 12;
     	   afwm = leftmask[srcx];
    	   alwm = rightmask[srcx2];
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

BOOL blit_copybox_mask(struct amigavideo_staticdata *data, struct amigabm_data *srcbm, struct amigabm_data *dstbm,
    WORD srcx, WORD srcy, WORD w, WORD h, WORD dstx, WORD dsty, HIDDT_DrawMode mode, APTR maskplane)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;
    ULONG srcoffset, dstoffset;
    BYTE shift, i;
    WORD srcwidth, dstwidth, width;
    WORD srcx2, dstx2;
    UWORD bltshift;
    UWORD afwm, alwm;
    UWORD bltcon0;
    BOOL reverse = FALSE;
    BOOL intersect = FALSE;

    if (USE_BLITTER == 0)
    	return FALSE;

    srcx2 = srcx + w - 1;
    dstx2 = dstx + w - 1;

    D(bug("CopyBoxMasked(%x %x %dx%d %dx%d %dx%d %d %x)\n", srcbm, dstbm, srcx, srcy, dstx, dsty, w, h, mode, maskplane));

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
    srcx &= 15;
    dstx &= 15;
    dstx2 &= 15;
    srcx2 &= 15;

    bltshift = shift << 12;
    if (reverse) {
     	if (dstwidth > srcwidth) {
    	    width = dstwidth;
     	    alwm = 0xffff;
    	    afwm = 0x0000;
    	} else {
    	    width = srcwidth;
     	    alwm = 0xffff;
    	    afwm = 0xffff;
    	}
    	srcoffset += srcbm->bytesperrow * (h - 1) + (width - 1) * 2;
    	dstoffset += dstbm->bytesperrow * (h - 1) + (width - 1) * 2;
    } else {
     	if (dstwidth > srcwidth) {
    	    width = dstwidth;
    	    afwm = 0xffff;
    	    alwm = 0x0000;
    	} else {
    	    width = srcwidth;
     	    afwm = 0xffff;
    	    alwm = 0xffff;
    	}
    }

    D(bug("shift=%d rev=%d sw=%d dw=%d sbpr=%d %04x %04x\n", shift, reverse, srcwidth, dstwidth, srcbm->bytesperrow, afwm, alwm));

    OwnBlitter();
    WaitBlit();

    custom->bltafwm = afwm;
    custom->bltalwm = alwm;
    custom->bltamod = srcbm->bytesperrow - width * 2;
    custom->bltbmod = srcbm->bytesperrow - width * 2;
    custom->bltcmod = dstbm->bytesperrow - width * 2;
    custom->bltdmod = dstbm->bytesperrow - width * 2;
    custom->bltcon1 = (reverse ? 0x0002 : 0x0000) | bltshift;
    bltcon0 = 0x0f00 | 0x00ca | bltshift;
    
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
    	custom->bltapt = (APTR)(maskplane + srcoffset);
    	custom->bltcpt = (APTR)(dstbm->planes[i] + dstoffset);
    	custom->bltdpt = (APTR)(dstbm->planes[i] + dstoffset);
    	startblitter(data, width, h);
    }

    WaitBlit();
    DisownBlitter();

    return TRUE;
}

static const UBYTE fill_minterm[] = { 0xca, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x00, 0x00, 0xca };

// copy: /AC + AB = (NABC | NANBC) | (ABNC | ABC) = 0xCA
// invert: /AC + A/C = (NABC | NANBC) | (ABNC | ANBNC) = 0x5A

// A-DAT = edge masking
// B-DAT = pixel value (ones or zeros, not used when invert mode)
// C     = source
// D     = destination

BOOL blit_fillrect(struct amigavideo_staticdata *data, struct amigabm_data *bm, WORD x1,WORD y1,WORD x2,WORD y2, HIDDT_Pixel pixel, HIDDT_DrawMode mode)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;
    ULONG offset;
    WORD width, height;
    UBYTE i;
    
    if (USE_BLITTER == 0)
    	return FALSE;
    
    D(bug("fillrect(%dx%d,%dx%d,%d,%d)\n", x1, y1, x2, y2, pixel, mode));
    D(bug("bm = %dx%dx%d bpr=%d\n", bm->width, bm->height, bm->depth, bm->bytesperrow));

    if (fill_minterm[mode] == 0)
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
    	if (bm->planes[i] != (UBYTE*)0x00000000 && bm->planes[i] != (UBYTE*)0xffffffff) {
	    WaitBlit();
	    custom->bltbdat = (pixel & 1) ? 0xffff : 0x0000;
	    custom->bltcpt = (APTR)(bm->planes[i] + offset);
	    custom->bltdpt = (APTR)(bm->planes[i] + offset);
	    startblitter(data, width, height);
	}
	pixel >>= 1;
    }

    WaitBlit();
    DisownBlitter();

    return TRUE;
}

#if 0
struct pHidd_BitMap_PutTemplate
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *Template;
    ULONG	    modulo;
    WORD    	    srcx;
    WORD	    x, y;
    WORD	    width, height;
    BOOL    	    inverttemplate;
};
#endif

// A-DAT = edge masking
// B     = template
// C     = destination
// D     = destination

BOOL blit_puttemplate(struct amigavideo_staticdata *data, struct amigabm_data *bm, struct pHidd_BitMap_PutTemplate *tmpl)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;
    OOP_Object *gc = tmpl->gc;
    HIDDT_Pixel	fgpen = GC_FG(tmpl->gc);
    HIDDT_Pixel bgpen = GC_BG(tmpl->gc);

    UBYTE type, i;
    BYTE shift;
    UWORD shifta, shiftb;
    UWORD afwm, alwm;
    ULONG srcoffset, dstoffset;
    WORD width, srcwidth, dstwidth;
    BOOL reverse;
    WORD height = tmpl->height;
    WORD dstx = tmpl->x;
    WORD dsty = tmpl->y;
    WORD dstx2 = tmpl->x + tmpl->width - 1;
    WORD srcx = tmpl->srcx;
    WORD srcx2 = srcx + tmpl->width - 1;

    if (USE_BLITTER == 0)
    	return FALSE;

    if (GC_COLEXP(gc) == vHidd_GC_ColExp_Transparent)
    	type = 0;
    else if (GC_DRMD(gc) == vHidd_GC_DrawMode_Invert)
     	type = 2;
    else
     	type = 4;
    if (tmpl->inverttemplate)
    	type++;

    D(bug("puttemplate: %x srcx=%d x=%d y=%d w=%d h=%d type=%d fg=%d bg=%d\n",
	tmpl->masktemplate, tmpl->srcx, tmpl->x, tmpl->y, tmpl->width, tmpl->height, type, fgpen, bgpen));

    srcoffset = (srcx / 16) * 2;
    dstoffset = bm->bytesperrow * dsty + (dstx / 16) * 2;

    srcwidth = srcx2 / 16 - srcx / 16 + 1;
    dstwidth = dstx2 / 16 - dstx / 16 + 1;
 
    dstx &= 15;
    dstx2 &= 15;
    srcx &= 15;
    srcx2 &= 15;
 
    shift = dstx - srcx;
 
    D(bug("shift=%d srcwidth=%d dstwidth=%d\n", shift, srcwidth, dstwidth));
 
    if (shift < 0) {
     	reverse = TRUE;
    	shift = -shift;
    	if (dstwidth >= srcwidth) {
    	    width = dstwidth;
    	    shifta = 0;
    	    shiftb = shift << 12;
    	    alwm = leftmask[dstx];
    	    afwm = rightmask[dstx2];
    	} else {
    	    width = srcwidth;
    	    shifta = shift << 12;
    	    shiftb = shift << 12;
     	    alwm = leftmask[srcx];
    	    afwm = rightmask[srcx2];
    	}
    	srcoffset += tmpl->modulo * (height - 1) + (width - 1) * 2;
    	dstoffset += bm->bytesperrow * (height - 1) + (width - 1) * 2;
    } else {
    	reverse = FALSE;
     	if (dstwidth >= srcwidth) {
    	    width = dstwidth;
    	    shifta = 0;
    	    shiftb = shift << 12;
    	    afwm = leftmask[dstx];
    	    alwm = rightmask[dstx2];
    	} else {
    	   width = srcwidth;
    	   shifta = shift << 12;
    	   shiftb = shift << 12;
     	   afwm = leftmask[srcx];
    	   alwm = rightmask[srcx2];
    	}
    }

    OwnBlitter();
    WaitBlit();

    custom->bltafwm = afwm;
    custom->bltalwm = alwm;
    custom->bltbmod = tmpl->modulo - width * 2;
    custom->bltcmod = bm->bytesperrow - width * 2;
    custom->bltdmod = bm->bytesperrow - width * 2;
    custom->bltadat = 0xffff;
    
    for (i = 0; i < bm->depth; i++) {
    	UBYTE minterm;
    	UWORD chmask, shiftbv;
    	UBYTE fg, bg, tg;

  	fg = fgpen & 1;
  	bg = bgpen & 1;
  	fgpen >>= 1;
  	bgpen >>= 1;

    	if (bm->planes[i] == (UBYTE*)0x00000000 || bm->planes[i] == (UBYTE*)0xffffffff)
  	    continue;

  	chmask = 0x0700;
  	shiftbv = shiftb;
 
 	/* not guaranteed to be correct, last time I played with minterms
 	 * was something like 20 years ago..
 	 */
  	switch (type)
  	{
  	    case 0: // JAM1:
  	    minterm = (NABC | NANBC) | (fg ? ABC | ABNC | ANBC : ANBC);
  	    break;
  	    case 1: // JAM1 | INVERSVID
 	    minterm = (NABC | NANBC) | (fg ? ANBC | ANBNC | ABC : ABC);
  	    break;
  	    case 2: // COMPLEMENT:
  	    minterm = (NABC | NANBC) | (ANBC | ABNC);
  	    break;
  	    case 3: // COMPLEMENT | INVERSVID:
  	    minterm = (NABC | NANBC) | (ABC | ANBNC);
  	    break;
   	    case 5: // JAM2 | INVERSVID
   	    tg = fg;
   	    fg = bg;
   	    bg = tg;
 	    case 4: // JAM2
  	    if (fg && bg) {
  	    	minterm = (NABC | NANBC) | (ABC | ABNC);
  	    	chmask = 0x0300;
  	    	shiftbv = 0;
  	    } else if (!fg && !bg) {
  	    	minterm = (NABC | NANBC);
  	    	chmask = 0x0300;
  	    	shiftbv = 0;
   	    } else if (fg) {
  	    	minterm = (NABC | NANBC) | (ABC | ABNC);
  	    } else {
  	    	minterm = (NABC | NANBC) | (ANBC | ANBNC);
  	    }
  	    break;
  	}
  	
    	WaitBlit();
	custom->bltcon0 = shifta | chmask | minterm;
	custom->bltcon1 = (reverse ? 0x0002 : 0x0000) | shiftbv;
	custom->bltbdat = 0xffff;
	custom->bltbpt = (APTR)(tmpl->masktemplate + srcoffset);
    	custom->bltcpt = (APTR)(bm->planes[i] + dstoffset);
    	custom->bltdpt = (APTR)(bm->planes[i] + dstoffset);
    	startblitter(data, width, height);
    }

    WaitBlit();
    DisownBlitter();

    return TRUE;
}

static UBYTE getminterm(UBYTE type, UBYTE fg, UBYTE bg)
{
	UBYTE minterm = 0, tg;
  	switch (type)
  	{
  	    case 0: // JAM1:
  	    minterm = (NABC | NANBC) | (fg ? ABC | ABNC | ANBC : ANBC);
  	    break;
  	    case 1: // JAM1 | INVERSVID
 	    minterm = (NABC | NANBC) | (fg ? ANBC | ANBNC | ABC : ABC);
  	    break;
  	    case 2: // COMPLEMENT:
  	    minterm = (NABC | NANBC) | (ANBC | ABNC);
  	    break;
  	    case 3: // COMPLEMENT | INVERSVID:
  	    minterm = (NABC | NANBC) | (ABC | ANBNC);
  	    break;
   	    case 5: // JAM2 | INVERSVID
   	    tg = fg;
   	    fg = bg;
   	    bg = tg;
 	    case 4: // JAM2
  	    if (fg && bg) {
  	    	minterm = (NABC | NANBC) | (ABC | ABNC);
  	    } else if (!fg && !bg) {
  	    	minterm = (NABC | NANBC);
   	    } else if (fg) {
  	    	minterm = (NABC | NANBC) | (ABC | ABNC);
  	    } else {
  	    	minterm = (NABC | NANBC) | (ANBC | ANBNC);
  	    }
  	    break;
  	}
  	return minterm;
}

BOOL blit_putpattern(struct amigavideo_staticdata *csd, struct amigabm_data *bm, struct pHidd_BitMap_PutPattern *pat)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;

    UBYTE type;
    UBYTE fgpen = GC_FG(pat->gc);
    UBYTE bgpen = GC_BG(pat->gc);

    UBYTE i;
    UWORD shifta, shiftb;
    UWORD afwm, alwm;
    ULONG dstoffset;
    WORD dstwidth;
    WORD patcnt;

    WORD height = pat->height;
    WORD dstx = pat->x;
    WORD dsty = pat->y;
    WORD dstx2 = pat->x + pat->width - 1;
    
    WORD patternymask =  pat->patternheight - 1;

    if (USE_BLITTER == 0)
    	return FALSE;

    if (pat->mask)
    	return FALSE;
    if (pat->patterndepth != 1)
    	return FALSE;

    if (GC_COLEXP(pat->gc) == vHidd_GC_ColExp_Transparent)
	type = 0;
    else if (GC_DRMD(pat->gc) == vHidd_GC_DrawMode_Invert)
	type = 2;
    else
	type = 4;
    if (pat->invertpattern)
    	type++;

   D(bug("PutPattern(%dx%d,%dx%d,mask=%x,mod=%d,masksrcx=%d,P=%x,%dx%d,h=%d,T=%d)\n",
	pat->x, pat->y, pat->width, pat->height,
	pat->mask, pat->maskmodulo, pat->masksrcx,
	pat->pattern, pat->patternsrcx, pat->patternsrcy, pat->patternheight, type));

    dstoffset = bm->bytesperrow * dsty + (dstx / 16) * 2;
    dstwidth = dstx2 / 16 - dstx / 16 + 1;

    dstx &= 15;
    dstx2 &= 15;

    shifta = 0;
    shiftb = 0;
    afwm = leftmask[dstx];
    alwm = rightmask[dstx2];

    OwnBlitter();
    WaitBlit();

    custom->bltafwm = afwm;
    custom->bltalwm = alwm;
    custom->bltcmod = (bm->bytesperrow - dstwidth * 2) + bm->bytesperrow * (pat->patternheight - 1);
    custom->bltdmod = (bm->bytesperrow - dstwidth * 2) + bm->bytesperrow * (pat->patternheight - 1);
    custom->bltadat = 0xffff;
    
    for (i = 0; i < bm->depth; i++, fgpen >>= 1, bgpen >>= 1) {
    	ULONG dstoffset2;
    	UBYTE minterm;
    	UWORD chmask;
    	UBYTE fg, bg;

    	if (bm->planes[i] == (UBYTE*)0x00000000 || bm->planes[i] == (UBYTE*)0xffffffff)
  	    continue;

  	fg = fgpen & 1;
  	bg = bgpen & 1;
    	
  	chmask = 0x0300;
 
 	minterm = getminterm(type, fg, bg);
 
   	WaitBlit();
   	custom->bltcon0 = shifta | chmask | minterm;
	custom->bltcon1 = shiftb;

	for(patcnt = 0, dstoffset2 = 0; patcnt < pat->patternheight; patcnt++, dstoffset2 += bm->bytesperrow) {
    	    UWORD blitheight = (height - patcnt + 1) / pat->patternheight;
    	    UWORD pattern = ((UWORD*)pat->pattern)[(pat->patternsrcy + patcnt) & patternymask];
    	    if (blitheight && pattern) {
   		WaitBlit();
	    	custom->bltbdat = pattern;
    	    	custom->bltcpt = (APTR)(bm->planes[i] + dstoffset + dstoffset2);
    	    	custom->bltdpt = (APTR)(bm->planes[i] + dstoffset + dstoffset2);
    	    	startblitter(csd, dstwidth, blitheight);
    	    }
    	}
    }

    WaitBlit();
    DisownBlitter();

    return TRUE;
}

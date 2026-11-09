/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/graphics.h>

#include <exec/libraries.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hidd/gfx.h>

#include "amigavideo_hidd.h"
#include "blitter.h"

#include <aros/debug.h>

#define ABC    0x80
#define ABNC   0x40
#define ANBC   0x20
#define ANBNC  0x10
#define NABC   0x08
#define NABNC  0x04
#define NANBC  0x02
#define NANBNC 0x01

/* BLTCON0 channel enables. A channel left disabled does not read memory: it
   takes its data from BLTADAT or BLTBDAT instead. */
#define USEA   0x0800
#define USEB   0x0400
#define USEC   0x0200
#define USED   0x0100

// Safety check for fast ram bitmaps
static BOOL canblit(struct BitMap *bm)
{
        WORD i;
        for (i = 0; i < bm->Depth; i++) {
                UBYTE *p = bm->Planes[i];
        if (p == (UBYTE*)0x00000000 || p == (UBYTE*)0xffffffff)
                        continue;
                // Lower 2M is guaranteed to be Chip RAM
                if ((IPTR)p < 0x00200000)
                        continue;
                // Re-implementations and UAE may have more Chip RAM.
                if (TypeOfMem(p) & MEMF_CHIP)
                        continue;
                return FALSE;
        }
        return USE_BLITTER != 0;
}

/* Validate before allocating masks or owning the blitter. Zero in a size
 * register encodes the hardware maximum, not an empty rectangle. */
static BOOL blitsize(struct amigavideo_staticdata *data, UWORD w, UWORD h)
{
    if (data->ecs_agnus)
        return (UWORD)(w - 1) < 2048 && (UWORD)(h - 1) < 32768;
    return (UWORD)(w - 1) < 64 && (UWORD)(h - 1) < 1024;
}

/* A single-row blit never uses its modulo to reach another row. */
static BOOL blitmodulo(LONG modulo, UWORD height)
{
    return height == 1 || modulo == (WORD)modulo;
}

/* size and both modulos of a blit are within what the hardware takes */
static BOOL blitfits(struct amigavideo_staticdata *data, UWORD w, UWORD h, LONG mod1, LONG mod2)
{
    return blitsize(data, w, h) && blitmodulo(mod1, h) && blitmodulo(mod2, h);
}

static void startblitter(struct amigavideo_staticdata *data, UWORD w, UWORD h)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    if (data->ecs_agnus) {
        custom->bltsizv = h;
        custom->bltsizh = w;
    } else {
        /* Every caller admits the complete operation before OwnBlitter. */
        custom->bltsize = (h << 6) | (w & 63);
    }
}
#define LEFTMASK(x)  ((UWORD)(0xffffU >> (x)))
#define RIGHTMASK(x) ((UWORD)(0xffffU << (15 - (x))))

static const UBYTE copy_minterm[] = { 0xff, 0x8a, 0x00, 0xca, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0xff, 0x00, 0x3a, 0x00, 0x00, 0xff };
/* Every entry carries /AC (NABC | NANBC = 0x0a), which is what leaves the
 * destination alone where the A channel's edge mask is zero. Without it a blit
 * clears the pixels its first and last word mask out.
 *
 * [1] = And (D = src AND dst): ABC is 0x80, so the entry is 0x8a.
 * [7] = Or (D = src OR dst): A(B + C) is 0xe0, so the entry is 0xea.
 * [12] = CopyInverted (D = NOT src): copy minterm 0xca with the B (source)
 * channel inverted -> 0x3a. Used by DrawImageState for an icon's selected
 * state; without it that blit fell back to the per-pixel software CopyBox
 * (~0.8s per icon on a 68000). */
// A-DAT = edge masking
// B     = source
// C     = destination
// D     = destination
//  0: zero (->fillrect)
//  1: src AND dst              /AC + ABC
//  2: src AND NOT dst          /AC + AB/C
//  3: src                      /AC + AB
//  4: NOT src AND dst          /AC + A/B/C
//  5: dst (nop)
//  6: src XOR dst              /AC + A
//  7: copy source and blit thru mask
//  8:
//  9:
// 10: NOT dst (->fillrect)
// 11:
// 12:
// 13:
// 14:
// 15: one (->fillrect)

BOOL blit_copybox(struct amigavideo_staticdata *data, struct BitMap *srcbm, struct BitMap *dstbm,
    WORD srcx, WORD srcy, UWORD w, UWORD h, WORD dstx, WORD dsty, HIDDT_DrawMode mode, UBYTE mask)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;
    ULONG srcoffset, dstoffset;
    BYTE shift, i;
    WORD srcwidth, dstwidth, width;
    LONG srcx2, dstx2;
    UWORD shifta, shiftb;
    UWORD afwm, alwm;
    UWORD bltcon0;
    BOOL reverse = FALSE;
    BOOL intersect = FALSE;
    BOOL opposite_rows = FALSE;
    LONG srcmod, dstmod;

    if (!w || !h)
        return TRUE;
    if (dstx < 0 || dsty < 0)
        return FALSE;
    srcx2 = srcx + w - 1;
    dstx2 = dstx + w - 1;
    if (copy_minterm[mode] == 0xff) {
        /* FillRect takes signed endpoint coordinates. */
        if (dstx2 > 32767 || dsty + h - 1 > 32767)
            return FALSE;
        return blit_fillrect(data, dstbm, dstx, dsty, dstx2, dsty + h - 1, 0, mode, mask);
    }

    if (copy_minterm[mode] == 0)
        return FALSE;

    if (srcx < 0 || srcy < 0 ||
        !canblit(srcbm) || (srcbm != dstbm && !canblit(dstbm)))
        return FALSE;

    D(bug("[AmigaVideo:Blitter] %s(%x %x %dx%d %dx%d %dx%d %d)\n", __func__, srcbm, dstbm, srcx, srcy, dstx, dsty, w, h, mode));

    if (srcbm == dstbm) {
        intersect = !(srcx > dstx2 || srcx2 < dstx || srcy > dsty + h - 1 || srcy + h - 1 < dsty);
    }
 
    srcoffset = srcbm->BytesPerRow * srcy + (srcx / 16) * 2;
    dstoffset = dstbm->BytesPerRow * dsty + (dstx / 16) * 2;
    shift = (dstx & 15) - (srcx & 15);

    if (intersect) {
        if ((dstoffset < srcoffset && shift < 0) ||
            (dstoffset > srcoffset && shift > 0)) {
            /* Keep overlapping scanlines on the software path. For distinct
             * rows the blitter can reverse horizontally while its modulo
             * advances vertically in the other direction. */
            if (srcy == dsty)
                return FALSE;
            opposite_rows = TRUE;
        }
    }

    if (shift < 0) {
        reverse = TRUE;
        shift = -shift;
    } else if (intersect && dstoffset > srcoffset && !opposite_rows) {
        reverse = TRUE;
    }

    srcwidth = srcx2 / 16 - srcx / 16 + 1;
    dstwidth = dstx2 / 16 - dstx / 16 + 1;

    srcx &= 15;
    dstx &= 15;
    dstx2 &= 15;
    srcx2 &= 15;

    shiftb = shift << 12;
    if (dstwidth >= srcwidth) {
        width = dstwidth;
        shifta = 0;
    } else {
        width = srcwidth;
        shifta = shiftb;
        dstx = srcx;
        dstx2 = srcx2;
    }
    afwm = LEFTMASK(dstx);
    alwm = RIGHTMASK(dstx2);
    if (reverse) {
        UWORD tmp = afwm;
        afwm = alwm;
        alwm = tmp;
        srcoffset += (width - 1) * 2;
        dstoffset += (width - 1) * 2;
    }

    /* Opposite row traversal only occurs within the same bitmap. */
    srcmod = (opposite_rows ? -srcbm->BytesPerRow : srcbm->BytesPerRow) - width * 2;
    dstmod = opposite_rows ? srcmod : dstbm->BytesPerRow - width * 2;
    if (!blitfits(data, width, h, srcmod, dstmod))
        return FALSE;

    /* Select the first row independently of the horizontal direction. */
    if (reverse != opposite_rows) {
        srcoffset += srcbm->BytesPerRow * (h - 1);
        dstoffset += dstbm->BytesPerRow * (h - 1);
    }

    D(bug("[AmigaVideo:Blitter] %s: shift=%d rev=%d sw=%d dw=%d %04x %04x\n", __func__, shift, reverse, srcwidth, dstwidth, afwm, alwm));

    OwnBlitter();
    WaitBlit();

    custom->bltafwm = afwm;
    custom->bltalwm = alwm;
    custom->bltbmod = srcmod;
    custom->bltcmod = dstmod;
    custom->bltdmod = dstmod;
    bltcon0 = USEB | USEC | USED | copy_minterm[mode] | shifta;
    custom->bltadat = 0xffff;
    
    for (i = 0; i < dstbm->Depth; i++) {
        UWORD bltcon0b = bltcon0;
        PLANEPTR source = i < srcbm->Depth ? srcbm->Planes[i] : NULL;
        /* The write mask protects the planes it clears. It is a UBYTE, so it
           says nothing about a ninth plane and beyond: leave those writable. */
        if (i < 8 && !(mask & (1 << i)))
            continue;
        if (dstbm->Planes[i] == (UBYTE*)0x00000000 || dstbm->Planes[i] == (UBYTE*)0xffffffff)
            continue;
        WaitBlit();
        if (!source || source == (PLANEPTR)-1) {
            bltcon0b &= ~USEB;
            custom->bltcon1 = reverse ? 0x0002 : 0x0000;
            custom->bltbdat = source ? 0xffff : 0x0000;
        } else {
            custom->bltcon1 = (reverse ? 0x0002 : 0x0000) | shiftb;
            custom->bltbpt = (APTR)(source + srcoffset);
        }
        /* A disabled B channel uses BLTBDAT repeatedly. Shifting that
           constant would drop bits at each word boundary instead of
           carrying them from the preceding DMA word. */
        custom->bltcon0 = bltcon0b;
        custom->bltcpt = (APTR)(dstbm->Planes[i] + dstoffset);
        custom->bltdpt = (APTR)(dstbm->Planes[i] + dstoffset);
        startblitter(data, width, h);
    }

    WaitBlit();
    DisownBlitter();

    return TRUE;
}

BOOL blit_copybox_mask(struct amigavideo_staticdata *data, struct BitMap *srcbm, struct BitMap *dstbm,
    WORD srcx, WORD srcy, UWORD w, UWORD h, WORD dstx, WORD dsty, HIDDT_DrawMode mode, APTR maskplane)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;
    ULONG srcoffset, dstoffset;
    BYTE shift, i;
    WORD srcwidth, dstwidth, width;
    LONG srcx2, dstx2;
    UWORD bltshift;
    UWORD afwm, alwm;
    UWORD bltcon0, maskmod;
    LONG srcmod, dstmod;
    BOOL reverse = FALSE;
    BOOL intersect = FALSE;
    UWORD *tempmask = NULL;
    BOOL needtempmask = FALSE;

    if (!w || !h)
        return TRUE;
    if (srcx < 0 || srcy < 0 || dstx < 0 || dsty < 0)
        return FALSE;
    if (!canblit(srcbm) || (srcbm != dstbm && !canblit(dstbm)))
        return FALSE;

    srcx2 = srcx + w - 1;
    dstx2 = dstx + w - 1;

    D(bug("[AmigaVideo:Blitter] %s(%x %x %dx%d %dx%d %dx%d %d %x)\n", __func__, srcbm, dstbm, srcx, srcy, dstx, dsty, w, h, mode, maskplane));

    if (srcbm == dstbm) {
        intersect = !(srcx > dstx2 || srcx2 < dstx || srcy > dsty + h - 1 || srcy + h - 1 < dsty);
    }
 
    srcoffset = srcbm->BytesPerRow * srcy + (srcx / 16) * 2;
    dstoffset = dstbm->BytesPerRow * dsty + (dstx / 16) * 2;
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

        if (dstwidth > srcwidth && ((srcx + w - 1) & ~15) != ((dstx + w - 1) & ~15)) {
                // >16 bit edge mask needed?
                D(bug("[AmigaVideo:Blitter] %s: unsupported %d,%d %d,%d %d %d,%d\n", __func__, srcwidth, dstwidth, srcx, dstx, w, srcx + w, dstx + w));
                needtempmask = TRUE;
        }

    bltshift = shift << 12;
    /* masks in source order; a descending blit meets them the other way round */
    afwm = LEFTMASK(srcx);
    alwm = RIGHTMASK(srcx2);
    if (dstwidth > srcwidth) {
        width = dstwidth;
        if (srcwidth == 1 && srcx2 > srcx) {
            // source is single word
            afwm &= alwm;
            alwm = 0xffff;
        }
    } else {
        width = srcwidth;
    }
    if (reverse) {
        UWORD tmp = afwm;
        afwm = alwm;
        alwm = tmp;
    }

    /* The temporary mask, when needed, has this same width and height. */
    srcmod = srcbm->BytesPerRow - width * 2;
    dstmod = dstbm->BytesPerRow - width * 2;
    if (!blitfits(data, width, h, srcmod, dstmod))
        return FALSE;

    if (reverse) {
        srcoffset += srcbm->BytesPerRow * (h - 1) + (width - 1) * 2;
        dstoffset += dstbm->BytesPerRow * (h - 1) + (width - 1) * 2;
    }

        if (needtempmask) {
                UWORD *oldmask = maskplane + srcoffset, *newmask, *mask2;
                WORD ww;
                tempmask = AllocVec((srcwidth + 1) * 2 * (h + 1), MEMF_CHIP);
                if (!tempmask)
                        return FALSE;
                mask2 = tempmask;
                newmask = tempmask + (srcwidth + 1);

                // create temp mask generation mask
                for (ww = 0; ww < srcwidth - 1; ww++)
                        mask2[ww] = 0xffff;
                // we need to mask out part of second to last word which
                // is not directly supported by the blitter.
                mask2[srcwidth - 1] = reverse ? afwm : alwm;
                mask2[srcwidth] = 0x0000;

                // create temp mask
                OwnBlitter();
                WaitBlit();
                custom->bltapt = mask2;
                custom->bltbdat = 0x0000;
                custom->bltcpt = oldmask;
                custom->bltdpt = newmask;
                custom->bltamod = -(srcwidth + 1) * 2;
                custom->bltcmod = srcmod;
                custom->bltdmod = 0;
                custom->bltafwm = 0xffff;
                custom->bltalwm = 0xffff;
                // AC+/AB
                custom->bltcon0 = USEA | USEC | USED | 0xac;
                custom->bltcon1 = 0x0000;
                startblitter(data, srcwidth + 1, h);
                DisownBlitter();

                maskplane = newmask;
                maskmod = 0;
        } else {
                maskplane += srcoffset;
                maskmod = srcmod;
        }

    D(bug("[AmigaVideo:Blitter] %s: shift=%d rev=%d sw=%d dw=%d sbpr=%d srcx=%d/%d %04x %04x %p\n", __func__, shift, reverse, srcwidth, dstwidth, srcbm->BytesPerRow, srcx, srcx2, afwm, alwm, maskplane));

    OwnBlitter();
    WaitBlit();

    custom->bltafwm = afwm;
    custom->bltalwm = alwm;
    custom->bltamod = maskmod;
    custom->bltbmod = srcmod;
    custom->bltcmod = dstmod;
    custom->bltdmod = dstmod;
    /* CopyBoxMasked's Or mode represents BltMaskBitMapRastPort's 0xe0:
       replacement through the mask, rather than OR with old destination bits.
       Both supported API modes therefore use the hardware cookie cut. */
    bltcon0 = USEA | USEB | USEC | USED | 0xca | bltshift;
    
    for (i = 0; i < dstbm->Depth; i++) {
        UWORD bltcon0b = bltcon0;
        PLANEPTR source = i < srcbm->Depth ? srcbm->Planes[i] : NULL;
        if (dstbm->Planes[i] == (UBYTE*)0x00000000 || dstbm->Planes[i] == (UBYTE*)0xffffffff)
            continue;
        WaitBlit();
        if (!source || source == (PLANEPTR)-1) {
            bltcon0b &= ~USEB;
            custom->bltcon1 = reverse ? 0x0002 : 0x0000;
            custom->bltbdat = source ? 0xffff : 0x0000;
        } else {
            custom->bltcon1 = (reverse ? 0x0002 : 0x0000) | bltshift;
            custom->bltbpt = (APTR)(source + srcoffset);
        }
        /* Keep the mask's A shift, but do not shift a constant B plane. */
        custom->bltcon0 = bltcon0b;
        custom->bltapt = (APTR)maskplane;
        custom->bltcpt = (APTR)(dstbm->Planes[i] + dstoffset);
        custom->bltdpt = (APTR)(dstbm->Planes[i] + dstoffset);
        startblitter(data, width, h);
    }

    WaitBlit();
    DisownBlitter();

        if (tempmask)
                FreeVec(tempmask);

    return TRUE;
}

static const UBYTE fill_minterm[] = { 0xca, 0x00, 0x00, 0xca, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x00, 0x00, 0xca };

// copy: /AC + AB = (NABC | NANBC) | (ABNC | ABC) = 0xCA
// invert: /AC + A/C = (NABC | NANBC) | (ABNC | ANBNC) = 0x5A

// A-DAT = edge masking
// B-DAT = pixel value (ones or zeros, not used when invert mode)
// C     = source
// D     = destination

BOOL blit_fillrect(struct amigavideo_staticdata *data, struct BitMap *bm, WORD x1,WORD y1,WORD x2,WORD y2, HIDDT_Pixel pixel, HIDDT_DrawMode mode, UBYTE mask)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;
    ULONG offset;
    UWORD width, height;
    LONG modulo;
    UBYTE i;
    
    if (x2 < x1 || y2 < y1)
        return TRUE;
    if (x1 < 0 || y1 < 0 || !canblit(bm))
        return FALSE;
    
    D(
      bug("[AmigaVideo:Blitter] %s(%dx%d,%dx%d,%d,%d)\n", __func__, x1, y1, x2, y2, pixel, mode);
      bug("[AmigaVideo:Blitter] %s: bm = %dx%dx%d bpr=%d\n", __func__, bm->BytesPerRow * 8, bm->Rows, bm->Depth, bm->BytesPerRow);
     )

    if (fill_minterm[mode] == 0)
        return FALSE;

    offset = bm->BytesPerRow * y1 + (x1 / 16) * 2;
    width = x2 / 16 - x1 / 16 + 1;
    height = y2 - y1 + 1;

    modulo = bm->BytesPerRow - width * 2;
    if (!blitfits(data, width, height, modulo, modulo))
        return FALSE;

    OwnBlitter();
    WaitBlit();

    custom->bltafwm = LEFTMASK(x1 & 15);
    custom->bltalwm = RIGHTMASK(x2 & 15);
    custom->bltcmod = modulo;
    custom->bltdmod = modulo;
    custom->bltcon1 = 0x0000;
    custom->bltcon0 = USEC | USED | fill_minterm[mode];
    custom->bltadat = 0xffff;
    
    if (mode == vHidd_GC_DrawMode_Clear)
        pixel = 0x00;
    else if (mode == vHidd_GC_DrawMode_Set || mode == vHidd_GC_DrawMode_Invert)
        pixel = 0xff;

    for (i = 0; i < bm->Depth; i++) {
        /* The write mask protects the planes it clears. It is a UBYTE, so it
           says nothing about a ninth plane and beyond: leave those writable. */
        if ((i >= 8 || (mask & (1 << i))) &&
            bm->Planes[i] != (UBYTE*)0x00000000 && bm->Planes[i] != (UBYTE*)0xffffffff) {
            WaitBlit();
            custom->bltbdat = (pixel & 1) ? 0xffff : 0x0000;
            custom->bltcpt = (APTR)(bm->Planes[i] + offset);
            custom->bltdpt = (APTR)(bm->Planes[i] + offset);
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
    OOP_Object      *gc;
    UBYTE           *Template;
    ULONG           modulo;
    WORD            srcx;
    WORD            x, y;
    WORD            width, height;
    BOOL            inverttemplate;
};
#endif

/*
 * Minterm of a template or pattern blit for one plane, indexed by
 * (type << 2) | (fg << 1) | bg. type: 0 JAM1, 2 COMPLEMENT, 4 JAM2, plus 1
 * for INVERSVID. fg and bg are the plane's bit of the two pens. Every entry
 * carries /AC, which leaves the destination alone outside the mask.
 */
#define MT_KEEP (NABC | NANBC)
static const UBYTE fill_minterms[] = {
    /* JAM1: the pattern's ones take the foreground pen */
    MT_KEEP | ANBC, MT_KEEP | ANBC,
    MT_KEEP | ABC | ABNC | ANBC, MT_KEEP | ABC | ABNC | ANBC,
    /* JAM1 | INVERSVID: the pattern's zeros take the foreground pen */
    MT_KEEP | ABC, MT_KEEP | ABC,
    MT_KEEP | ANBC | ANBNC | ABC, MT_KEEP | ANBC | ANBNC | ABC,
    /* COMPLEMENT */
    MT_KEEP | ANBC | ABNC, MT_KEEP | ANBC | ABNC,
    MT_KEEP | ANBC | ABNC, MT_KEEP | ANBC | ABNC,
    /* COMPLEMENT | INVERSVID */
    MT_KEEP | ABC | ANBNC, MT_KEEP | ABC | ANBNC,
    MT_KEEP | ABC | ANBNC, MT_KEEP | ABC | ANBNC,
    /* JAM2: ones take the foreground pen, zeros the background pen */
    MT_KEEP, MT_KEEP | ANBC | ANBNC,
    MT_KEEP | ABC | ABNC, MT_KEEP | ABC | ABNC | ANBC | ANBNC,
    /* JAM2 | INVERSVID: the pens exchanged */
    MT_KEEP, MT_KEEP | ABC | ABNC,
    MT_KEEP | ANBC | ANBNC, MT_KEEP | ABC | ABNC | ANBC | ANBNC
};

// A-DAT = edge masking
// B     = template
// C     = destination
// D     = destination

BOOL blit_puttemplate(struct amigavideo_staticdata *data, struct BitMap *bm, struct pHidd_BitMap_PutTemplate *tmpl)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;
    OOP_Object *gc = tmpl->gc;
    HIDDT_Pixel fgpen = GC_FG(tmpl->gc);
    HIDDT_Pixel bgpen = GC_BG(tmpl->gc);
    ULONG colmask = GC_COLMASK(tmpl->gc);

    UBYTE type, i;
    BYTE shift;
    UWORD shifta, shiftb;
    UWORD afwm, alwm;
    ULONG srcoffset, dstoffset;
    WORD width, srcwidth, dstwidth;
    LONG srcmod, dstmod;
    BOOL reverse;
    WORD height = tmpl->height;
    WORD dstx = tmpl->x;
    WORD dsty = tmpl->y;
    LONG dstx2 = tmpl->x + tmpl->width - 1;
    WORD srcx = tmpl->srcx;
    LONG srcx2 = srcx + tmpl->width - 1;

    if (tmpl->width <= 0 || height <= 0 || srcx < 0 || dstx < 0 || dsty < 0 || !canblit(bm))
        return FALSE;

    if (GC_COLEXP(gc) == vHidd_GC_ColExp_Transparent)
        type = 0;
    else if (GC_DRMD(gc) == vHidd_GC_DrawMode_Invert)
        type = 2;
    else
        type = 4;
    if (tmpl->inverttemplate)
        type++;

    D(bug("[AmigaVideo:Blitter] %s: %x srcx=%d x=%d y=%d w=%d h=%d type=%d fg=%d bg=%d\n",
        __func__, tmpl->masktemplate, tmpl->srcx, tmpl->x, tmpl->y, tmpl->width, tmpl->height, type, fgpen, bgpen));

    srcoffset = (srcx / 16) * 2;
    dstoffset = bm->BytesPerRow * dsty + (dstx / 16) * 2;

    srcwidth = srcx2 / 16 - srcx / 16 + 1;
    dstwidth = dstx2 / 16 - dstx / 16 + 1;
 
    dstx &= 15;
    dstx2 &= 15;
    srcx &= 15;
    srcx2 &= 15;
 
    shift = dstx - srcx;
 
    D(bug("[AmigaVideo:Blitter] %s: shift=%d srcwidth=%d dstwidth=%d\n", __func__, shift, srcwidth, dstwidth));
 
    reverse = shift < 0;
    if (reverse)
        shift = -shift;
    shiftb = shift << 12;
    if (dstwidth >= srcwidth) {
        width = dstwidth;
        shifta = 0;
        afwm = LEFTMASK(dstx);
        alwm = RIGHTMASK(dstx2);
    } else {
        width = srcwidth;
        shifta = shiftb;
        afwm = LEFTMASK(srcx);
        alwm = RIGHTMASK(srcx2);
    }
    if (reverse) {
        UWORD tmp = afwm;
        afwm = alwm;
        alwm = tmp;
        srcoffset += tmpl->modulo * (height - 1) + (width - 1) * 2;
        dstoffset += bm->BytesPerRow * (height - 1) + (width - 1) * 2;
    }

    srcmod = tmpl->modulo - width * 2;
    dstmod = bm->BytesPerRow - width * 2;
    if (!blitfits(data, width, height, srcmod, dstmod))
        return FALSE;

    OwnBlitter();
    WaitBlit();

    custom->bltafwm = afwm;
    custom->bltalwm = alwm;
    custom->bltbmod = srcmod;
    custom->bltcmod = dstmod;
    custom->bltdmod = dstmod;
    custom->bltadat = 0xffff;
    
    for (i = 0; i < bm->Depth; i++) {
        UBYTE minterm;
        UWORD chmask, shiftbv;
        UBYTE fg, bg;

        fg = fgpen & 1;
        bg = bgpen & 1;
        fgpen >>= 1;
        bgpen >>= 1;

        if (bm->Planes[i] == (UBYTE*)0x00000000 || bm->Planes[i] == (UBYTE*)0xffffffff)
            continue;

        /* The write mask protects the planes it clears. It is a UBYTE, so it
           says nothing about a ninth plane and beyond: leave those writable. */
        if (i < 8 && !(colmask & (1 << i)))
            continue;

        chmask = USEB | USEC | USED;
        shiftbv = shiftb;
 
        minterm = fill_minterms[(type << 2) | (fg << 1) | bg];
        if (type >= 4 && fg == bg) {
            /* JAM2 with both pens alike in this plane: a solid fill, so the
               template is not fetched */
            if (fg)
                minterm = (NABC | NANBC) | (ABC | ABNC);
            chmask = USEC | USED;
            shiftbv = 0;
        }
 
        WaitBlit();
        custom->bltcon0 = shifta | chmask | minterm;
        custom->bltcon1 = (reverse ? 0x0002 : 0x0000) | shiftbv;
        custom->bltbdat = 0xffff;
        custom->bltbpt = (APTR)(tmpl->masktemplate + srcoffset);
        custom->bltcpt = (APTR)(bm->Planes[i] + dstoffset);
        custom->bltdpt = (APTR)(bm->Planes[i] + dstoffset);
        startblitter(data, width, height);
    }

    WaitBlit();
    DisownBlitter();

    return TRUE;
}

BOOL blit_putpattern(struct amigavideo_staticdata *csd, struct BitMap *bm, struct pHidd_BitMap_PutPattern *pat)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;

    UBYTE type;
    UBYTE fgpen = GC_FG(pat->gc);
    UBYTE bgpen = GC_BG(pat->gc);
    ULONG colmask = GC_COLMASK(pat->gc);

    UBYTE i;
    UWORD shifta, shiftb;
    UWORD afwm, alwm;
    ULONG maskoffset = 0, dstoffset;
    WORD width, maskwidth = 0, dstwidth;
    WORD patcnt;
    BOOL reverse = FALSE;

    WORD height = pat->height;
    WORD dstx = pat->x;
    WORD dsty = pat->y;
    LONG dstx2 = pat->x + pat->width - 1;
    
    WORD patternymask =  pat->patternheight - 1;
    UWORD maxheight;
    ULONG dststep, maskstep = 0;
    LONG dstmod, maskmod = 0;

    if (pat->patternheight <= 0)
        return TRUE;
    if (pat->width <= 0 || height <= 0 || pat->patterndepth != 1 ||
        dstx < 0 || dsty < 0 || (pat->mask && pat->masksrcx < 0) || !canblit(bm))
        return FALSE;

    if (GC_COLEXP(pat->gc) == vHidd_GC_ColExp_Transparent)
        type = 0;
    else if (GC_DRMD(pat->gc) == vHidd_GC_DrawMode_Invert)
        type = 2;
    else
        type = 4;
    if (pat->invertpattern)
        type++;

   D(bug("[AmigaVideo:Blitter] %s(%dx%d,%dx%d,mask=%x,mod=%d,masksrcx=%d,P=%x,%dx%d,h=%d,T=%d)\n",
        __func__, pat->x, pat->y, pat->width, pat->height,
        pat->mask, pat->maskmodulo, pat->masksrcx,
        pat->pattern, pat->patternsrcx, pat->patternsrcy, pat->patternheight, type));

    dstoffset = bm->BytesPerRow * dsty + (dstx / 16) * 2;
    dstwidth = dstx2 / 16 - dstx / 16 + 1;

    dstx &= 15;
    dstx2 &= 15;

    shifta = 0;
    shiftb = 0;
    width = dstwidth;
    afwm = LEFTMASK(dstx);
    alwm = RIGHTMASK(dstx2);

    if (pat->mask) {
        WORD maskx = pat->masksrcx;
        LONG maskx2 = maskx + pat->width - 1;
        BYTE shift;

        maskoffset = (maskx / 16) * 2;
        maskwidth = maskx2 / 16 - maskx / 16 + 1;
        maskx &= 15;
        maskx2 &= 15;
        shift = dstx - maskx;

        /*
         * The word masks trim channel A, which carries the mask and is read
         * before the shifter, so they have to be expressed where A's words
         * are - in the mask's coordinates, not the destination's. The two
         * differ by exactly the shift. Trimming there covers the destination
         * as well: where A is clear the minterm leaves D as C, so a bit the
         * mask does not reach is a bit the blit does not write.
         *
         * Whichever operand spans more words sets the blit's width, and when
         * that is the destination the final word has no mask behind it. There
         * is nothing to keep from it, and reading past the mask would shift
         * whatever follows into the pixels after the rectangle.
         */
        width = dstwidth >= maskwidth ? dstwidth : maskwidth;
        shifta = shift < 0 ? (-shift) << 12 : shift << 12;

        if (shift < 0) {
            reverse = TRUE;
            afwm = RIGHTMASK(maskx2);
            alwm = width > maskwidth ? 0 : LEFTMASK(maskx);
        } else {
            afwm = LEFTMASK(maskx);
            alwm = width > maskwidth ? 0 : RIGHTMASK(maskx2);
        }
    }

    /* Check every phase before drawing any of them: falling back after a
     * partial invert would invert those pixels twice. */
    maxheight = (UWORD)(height - 1) / (UWORD)pat->patternheight + 1;
    dststep = bm->BytesPerRow * (UWORD)pat->patternheight;
    dstmod = dststep - width * 2;
    if (!blitfits(csd, width, maxheight, dstmod, dstmod))
        return FALSE;
    if (pat->mask) {
        maskstep = pat->maskmodulo * pat->patternheight;
        maskmod = maskstep - width * 2;
        /* Check the unmultiplied stride too, so a large product cannot
         * wrap back into range. Signed negative strides remain valid. */
        if (!blitfits(csd, width, maxheight, pat->maskmodulo - width * 2, maskmod))
            return FALSE;
    }

    OwnBlitter();
    WaitBlit();

    custom->bltafwm = afwm;
    custom->bltalwm = alwm;
    if (pat->mask)
        custom->bltamod = maskmod;
    custom->bltcmod = dstmod;
    custom->bltdmod = dstmod;
    custom->bltadat = 0xffff;
    
    for (i = 0; i < bm->Depth; i++, fgpen >>= 1, bgpen >>= 1) {
        ULONG dstoffset2;
        UBYTE minterm;
        UWORD chmask;
        UBYTE fg, bg;

        if (bm->Planes[i] == (UBYTE*)0x00000000 || bm->Planes[i] == (UBYTE*)0xffffffff)
            continue;

        /* The write mask protects the planes it clears. It is a UBYTE, so it
           says nothing about a ninth plane and beyond: leave those writable. */
        if (i < 8 && !(colmask & (1 << i)))
            continue;

        fg = fgpen & 1;
        bg = bgpen & 1;

        /* The pattern is a single word in BLTBDAT, so B is never fetched from
           memory: enable A only when there is a mask to read. */
        chmask = pat->mask ? (USEA | USEC | USED) : (USEC | USED);
 
        minterm = fill_minterms[(type << 2) | (fg << 1) | bg];
 
        WaitBlit();
        custom->bltcon0 = shifta | chmask | minterm;
        custom->bltcon1 = (reverse ? 0x0002 : 0x0000) | shiftb;

        for(patcnt = 0, dstoffset2 = 0; patcnt < pat->patternheight; patcnt++, dstoffset2 += bm->BytesPerRow) {
            /* Rows patcnt, patcnt + patternheight, ... that fall inside the
               fill: a round up, not the round to nearest that only came out
               right for a two-row pattern. */
            UWORD blitheight = (UWORD)(height - patcnt + pat->patternheight - 1) / (UWORD)pat->patternheight;
            UWORD pattern = ((UWORD*)pat->pattern)[(pat->patternsrcy + patcnt) & patternymask];
            UWORD patternshift = (dstx - pat->patternsrcx) & 15;

            if (patternshift)
                pattern = (pattern >> patternshift) |
                          (pattern << (16 - patternshift));
            if (blitheight) {
                ULONG planeoffset = dstoffset + dstoffset2;
                ULONG sourceoffset = maskoffset + patcnt * pat->maskmodulo;

                if (reverse) {
                    planeoffset += (blitheight - 1) * dststep +
                                   (width - 1) * 2;
                    sourceoffset += (blitheight - 1) * maskstep +
                                    (width - 1) * 2;
                }
                WaitBlit();
                custom->bltbdat = pattern;
                if (pat->mask)
                    custom->bltapt = (APTR)(pat->mask + sourceoffset);
                custom->bltcpt = (APTR)(bm->Planes[i] + planeoffset);
                custom->bltdpt = (APTR)(bm->Planes[i] + planeoffset);
                startblitter(csd, width, blitheight);
            }
        }
    }

    WaitBlit();
    DisownBlitter();

    return TRUE;
}

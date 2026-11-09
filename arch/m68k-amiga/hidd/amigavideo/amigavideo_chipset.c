/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cia.h>

#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/types.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hardware/cia.h>
#include <hidd/gfx.h>
#include <graphics/modeid.h>
#include <graphics/sprite.h>

#include "amigavideo_hidd.h"
#include "amigavideo_bitmap.h"
#include "chipset.h"

#define BPLCONMASKFULL          0xFA55
#define BPLCONMASK              0x8A55

#define DEFAULT_BORDER_GRAY     0x0AAA

static const UBYTE fetchunits[] = { 3,3,3,0, 4,3,3,0, 5,4,3,0 };
static const UBYTE fm_maxplanes[] = { 3,2,1,0, 3,3,2,0, 3,3,3,0 };

/*
 * Select the widest AGA bitplane fetch supported by BytesPerRow and every
 * displayed plane address.  On standard AGA machines GfxBase->MemType is
 * BANDWIDTH_4X, matching the layout-dependent part of the Kickstart 3.1
 * selector.  The values used here are internal indices: 0, 1 and 2 map to
 * 16-, 32- and 64-bit fetches; the latter is written to FMODE as 3 by
 * setfmode().
 */
static BYTE bestbplfetchmode(struct amigabm_data *bm)
{
    IPTR alignment = bm->bytesperrow;
    UBYTE i;

    for (i = 0; i < bm->depth; i++)
        alignment |= (IPTR)bm->pbm->Planes[i];

    if (alignment & 1)
        return -1;
    if (!(alignment & 7))
        return 2;
    if (!(alignment & 3))
        return 1;
    return 0;
}

/* the visible height of a screen: at most the nominal display height, and
   never more than its ViewPort */
VOID setdisplayheight(struct amigavideo_staticdata *csd, struct amigabm_data *bm, WORD y)
{
    bm->displayheight = limitheight(csd, y, bm->interlace, FALSE);
    if (bm->displayheight > bm->viewportheight)
        bm->displayheight = bm->viewportheight;
}

/* give a non-interlaced screen a short frame list: a copy of its long frame one */
VOID initvpicopper(struct ViewPort *vp, struct amigabm_data *bm)
{
    if (!(vp->DspIns->CopSStart))
        vp->DspIns->CopSStart = AllocVec((vp->DspIns->MaxCount << 2), MEMF_CLEAR | MEMF_CHIP);

    D(bug("[AmigaVideo] %s: dummy CopSStart @ 0x%p\n", __func__, vp->DspIns->CopSStart);)

    /* copy the copperlist data ... */
    CopyMemQuick(vp->DspIns->CopLStart, vp->DspIns->CopSStart, (vp->DspIns->MaxCount << 2));

    /* copy adjusted pointers ... */
    bm->copsd.copper2_palette = vp->DspIns->CopSStart + (bm->copld.copper2_palette - vp->DspIns->CopLStart);
    bm->copsd.copper2_scroll = vp->DspIns->CopSStart + (bm->copld.copper2_scroll - vp->DspIns->CopLStart);
    bm->copsd.copper2_diwstop = vp->DspIns->CopSStart + (bm->copld.copper2_diwstop - vp->DspIns->CopLStart);
    bm->copsd.copper2_bpl = vp->DspIns->CopSStart + (bm->copld.copper2_bpl - vp->DspIns->CopLStart);
    bm->copsd.copper2_tail = vp->DspIns->CopSStart + (bm->copld.copper2_tail - vp->DspIns->CopLStart);
    bm->copsd.extralines = bm->copld.extralines;

    if (bm->copld.copper2_palette_aga_lo)
        bm->copsd.copper2_palette_aga_lo = vp->DspIns->CopSStart + (bm->copld.copper2_palette_aga_lo - vp->DspIns->CopLStart);
    if (bm->copld.copper2_fmode)
        bm->copsd.copper2_fmode = vp->DspIns->CopSStart + (bm->copld.copper2_fmode - vp->DspIns->CopLStart);
}

/* reset to OCS defaults */
VOID resetcustom(struct amigavideo_staticdata *csd)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;

    D(
      bug("[AmigaVideo] %s()\n", __func__);
      bug("[AmigaVideo] %s: GfxBase @ 0x%p\n", __func__, GfxBase);
     )

    GfxBase->system_bplcon0 &= ~BPLCONMASKFULL;
    GfxBase->system_bplcon0 |= 0x0200;
    D(bug("[AmigaVideo] %s: system_bplcon0 = %04x\n", __func__, GfxBase->system_bplcon0));

    if (csd->ecs_denise) {
        custom->fmode = 0x0000;
    }
    custom->bplcon0 = GfxBase->system_bplcon0;
    if (csd->ecs_denise) {
        csd->bplcon0_null = GfxBase->system_bplcon0 | 0x1;
    }
    else {
        csd->bplcon0_null = GfxBase->system_bplcon0;
    }
    csd->bplcon1 = 0x0000;
    custom->bplcon1 = csd->bplcon1;
    csd->bplcon2 = 0x0024;
    custom->bplcon2 = csd->bplcon2;
    csd->bplcon3 = 0x0c00;          /* set PF2OFx to default offset value 8 (plane 3 affected) */
    custom->bplcon3 = csd->bplcon3;

    // Use AGA modes and create AGA copperlists only if AGA is "enabled"
    csd->aga_enabled = csd->aga && (GfxBase->ChipRevBits0 & SETCHIPREV_AA) == SETCHIPREV_AA;

    csd->bplcon4 = 0x0011;          /* set OSPRM7 to default */
    if (csd->aga_enabled) {
        custom->bplcon4 = csd->bplcon4;
    }
    custom->vposw = 0x8000;
    custom->color[0] = DEFAULT_BORDER_GRAY;
}

/* point a sprite's copper1 pointer at p if the sprite is visible, at the
   empty sprite if not */
static VOID pointsprite(struct amigavideo_staticdata *csd, UWORD *spritept, UWORD *p, WORD spritey, BOOL visible)
{
    if (!spritept)
        return;
    if (visible) {
        struct amigabm_data *bm;
        ForeachNode(csd->compositedbms, bm)
        {
            /* A scrolled screen (negative topedge) starts its display band at the top */
            WORD bmtop = bm->topedge < 0 ? 0 : bm->topedge;

            if (spritey < ((bmtop + bm->displayheight) >> bm->interlace))
            {
                setfmode(csd, bm);
                break;
            }
        }
    } else {
        p = csd->sprite_null;
    }
    spritept[0] = (UWORD)(((ULONG)p) >> 16);
    spritept[2] = (UWORD)(((ULONG)p) >> 0);
}

#define setnullsprite(csd) pointsprite(csd, (csd)->copper1_spritept, NULL, 0, FALSE)
#define new_setnullsprite(csd, spritenum) pointsprite(csd, (csd)->new_copper1_spritept[spritenum], NULL, 0, FALSE)

VOID resetsprite(struct amigavideo_staticdata *csd)
{
    UWORD *sprite = csd->sprite;
    setnullsprite(csd);
    csd->sprite = NULL;
    FreeMem(sprite, csd->spritedatasize);
    csd->sprite_width = csd->sprite_height = 0;
}

VOID new_resetsprite(struct amigavideo_staticdata *csd, int spritenum)
{
    UWORD *sprite = csd->new_sprite[spritenum];
    new_setnullsprite(csd, spritenum);
    csd->new_sprite[spritenum] = NULL;
    FreeMem(sprite, csd->new_spritedatasize[spritenum]);
    csd->new_sprite_width[spritenum] = csd->new_sprite_height[spritenum] = 0;
}

VOID setfmode(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    UWORD fmode;
    fmode  =  csd->fmode_bpl == 2 ? 3 : csd->fmode_bpl;
    fmode |= (csd->fmode_spr == 2 ? 3 : csd->fmode_spr) << 2;
    if (bm && bm->copld.copper2_fmode) {
        *bm->copld.copper2_fmode = fmode;
        if (bm->interlace)
            *bm->copsd.copper2_fmode = fmode;
    }
    // Update sprite fmode in copper list 1 so the control words
    // are read correctly.
    csd->copper1[15] = fmode & 0xc;
}

static void updatecopper1frombm(struct amigavideo_staticdata *, struct amigabm_data *);
static VOID setcopperscrollbm(struct amigavideo_staticdata *, struct amigabm_data *);

VOID setcoppercolors(struct amigavideo_staticdata *csd, struct amigabm_data *bm, UBYTE *palette, BOOL update_copper1)
{
    struct copper2data *c2d = &bm->copld, *c2di = &bm->copsd;

    D(bug("[AmigaVideo] %s()\n", __func__));

    if (!(palette))
    {
        D(bug("[AmigaVideo] %s: missing palette data!\n", __func__));
        return;
    }
    if (!c2d->copper2_palette)
        return;

    /*
        The short frame list is built lazily, and its pointers are only
        refreshed from the long frame ones when it is (re)built - so an
        interlaced bitmap can reach us with some of them still NULL.
    */
    const BOOL doilace = bm->interlace && c2di->copper2_palette;

    UWORD i;

    /*
        Pick the branch that matches how this copper list was actually built,
        not the current chipset state: aga_enabled can be switched on long
        after the list exists (see the AGA-only mode hack in setmode()), and
        the lo half of the palette is only emitted for lists built with AGA
        already enabled. Trusting aga_enabled here would store through a NULL
        copper2_palette_aga_lo, which lands in the first page of memory.
    */
    if (csd->aga && csd->aga_enabled && c2d->copper2_palette_aga_lo) {
        UWORD off = 1;
        D(bug("[AmigaVideo] %s: AGA\n", __func__));
        for (i = 0; i < bm->use_colors; i++) {
            UWORD vallo, valhi;
            UBYTE r = palette[i * 3 + 0];
            UBYTE g = palette[i * 3 + 1];
            UBYTE b = palette[i * 3 + 2];
            if ((i & 31) == 0)
                off += 2;
            valhi = ((r & 0xf0) << 4) | ((g & 0xf0)) | ((b & 0xf0) >> 4);
            vallo = ((r & 0x0f) << 8) | ((g & 0x0f) << 4) | ((b & 0x0f));
            c2d->copper2_palette[i * 2 + off] = valhi;
            c2d->copper2_palette_aga_lo[i * 2 + off] = vallo;
            if (doilace && c2di->copper2_palette_aga_lo) {
                c2di->copper2_palette[i * 2 + off] = valhi;
                c2di->copper2_palette_aga_lo[i * 2 + off] = vallo;
            }
        }
    } else if (bm->res == 2 && !csd->aga) {
        D(bug("[AmigaVideo] %s: ECS\n", __func__));
        /* ECS "scrambled" superhires */
        for (i = 0; i < bm->use_colors; i++) {
            UBYTE offset = i < 16 ? 0 : 16;
            UBYTE c1 = (i & 3) + offset;
            UBYTE c2 = ((i >> 2) & 3) + offset;
            UWORD val1 = ((palette[c1 * 3 + 0] >> 4) << 8) | ((palette[c1 * 3 + 1] >> 4) << 4) | ((palette[c1 * 3 + 2] >> 4) << 0);
            UWORD val2 = ((palette[c2 * 3 + 0] >> 4) << 8) | ((palette[c2 * 3 + 1] >> 4) << 4) | ((palette[c2 * 3 + 2] >> 4) << 0);
            UWORD val = (val1 & 0xccc) | ((val2 & 0xccc) >> 2);
            c2d->copper2_palette[i * 2 + 1] = val;
            if (doilace)
                c2di->copper2_palette[i * 2 + 1] = val;
        }
        
    } else {
        for (i = 0; i < bm->use_colors; i++) {
            UWORD val = ((palette[i * 3 + 0] >> 4) << 8) | ((palette[i * 3 + 1] >> 4) << 4) | ((palette[i * 3 + 2] >> 4) << 0);
            c2d->copper2_palette[i * 2 + 1] = val;
            if (doilace)
                c2di->copper2_palette[i * 2 + 1] = val;
        }
    }
    if (update_copper1) {
        updatecopper1frombm(csd, bm);
    }
    D(bug("[AmigaVideo] %s: copper colors set\n", __func__));
}

VOID resetmode(struct amigavideo_staticdata *csd)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;

    D(bug("[AmigaVideo] %s()\n", __func__));

    custom->dmacon = 0x0100;
    csd->palmode = (GfxBase->DisplayFlags & NTSC) == 0;
    setpalntsc(csd);

    GfxBase->LOFlist = GfxBase->SHFlist = csd->copper2_backup;
    custom->cop2lc = (ULONG)csd->copper2_backup;
    custom->copjmp2 = 0;

    resetcustom(csd);
}

static VOID setcopperplanes(struct amigavideo_staticdata *csd, struct amigabm_data *bm,  struct copper2data *c2d, LONG offset, BOOL odd)
{
    UWORD *copbpl = c2d->copper2_bpl;
    WORD i;

    for (i = 0; i < bm->depth; i++) {
        ULONG pptr = (ULONG)(bm->pbm->Planes[bm->bploffsets[i]]);
        if (bm->interlace && odd)
            pptr += bm->bytesperrow;
        pptr += offset;
        copbpl[1] = (UWORD)(pptr >> 16);
        copbpl[3] = (UWORD)(pptr >> 0);
        copbpl += 4;
    }
}

static VOID changecopperplaneslist(struct amigabm_data *bm,
                                   struct copper2data *c2d,
                                   struct BitMap *oldbm,
                                   struct BitMap *newbm)
{
    UWORD *copbpl = c2d->copper2_bpl;
    WORD i;

    if (!copbpl)
        return;

    for (i = 0; i < bm->depth; i++)
    {
        UBYTE plane = bm->bploffsets[i];
        ULONG oldptr = (ULONG)oldbm->Planes[plane];
        ULONG newptr = (ULONG)newbm->Planes[plane];
        ULONG displayed = ((ULONG)copbpl[1] << 16) | copbpl[3];

        displayed += newptr - oldptr;
        copbpl[1] = displayed >> 16;
        copbpl[3] = displayed;
        copbpl += 4;
    }
}

static UWORD currentvpos(VOID)
{
    volatile struct Custom *custom = (struct Custom *)0xdff000;
    UWORD high, low;

    do
    {
        low = custom->vhposr >> 8;
        high = custom->vposr;
    } while (low != (custom->vhposr >> 8));

    return low | ((high & 7) << 8);
}

/*
 * Worst-case time budget for publishing a complete palette and plane-pointer
 * update before the copper reaches this viewport.  This is a relative beam
 * distance, not a screen-mode coordinate.
 */
#define COPPER_CHANGE_GUARD_LINES 32

static BOOL copperchangedanger(struct amigabm_data *bm)
{
    WORD displaystart = bm->diwstarty + (bm->topedge >> bm->interlace);
    WORD distance = (WORD)currentvpos() - displaystart;

    return distance < 0 && distance >= -COPPER_CHANGE_GUARD_LINES;
}

VOID commitcopperchanges(struct amigavideo_staticdata *csd,
                         struct amigabm_data *bm, BOOL wait_for_safe)
{
    BOOL topmost = csd->compositedbms != NULL &&
        (void *)bm == (void *)csd->compositedbms->lh_Head;

    for (;;)
    {
        if (wait_for_safe)
        {
            while (copperchangedanger(bm))
                ;
        }

        if (wait_for_safe)
            Disable();
        if (!wait_for_safe || !copperchangedanger(bm))
        {
            if (bm->palette_changed)
            {
                setcoppercolors(csd, bm, bm->palette, topmost);
                bm->palette_changed = FALSE;
            }

            if (bm->bitmap_changed && bm->displayed_pbm && bm->pbm)
            {
                changecopperplaneslist(bm, &bm->copld,
                                       bm->displayed_pbm, bm->pbm);
                if (bm->interlace)
                    changecopperplaneslist(bm, &bm->copsd,
                                           bm->displayed_pbm, bm->pbm);
                bm->displayed_pbm = bm->pbm;
                bm->bitmap_changed = FALSE;
            }

            if (wait_for_safe)
                Enable();
            break;
        }
        if (wait_for_safe)
            Enable();
    }
}

static VOID setcopperscroll2(struct amigavideo_staticdata *csd, struct amigabm_data *bm, struct copper2data *c2d, UWORD *c2, BOOL odd)
{
    UWORD *copptr = c2d->copper2_scroll;
    WORD xscroll, yscroll;
    WORD x, y;
    WORD ystart, yend, xdelay;
    WORD xmaxscroll,  modulo, ddfstrt, fmodewidth, minearly;
    LONG offset;

    D(bug("[AmigaVideo] %s(0x%p)\n", __func__, c2));

    fmodewidth = 16 << csd->fmode_bpl;
    x = bm->leftedge;
    y = bm->diwstarty + (bm->topedge >> bm->interlace);

    /* A screen positioned above the view top is scrolled, not moved: pin the
       display window at the top and turn the surplus into a bitplane offset,
       so the lower part of an oversized bitmap comes into view. */
    yscroll = 0;
    if (y < bm->diwstarty) {
        yscroll = y - bm->diwstarty;
        y = bm->diwstarty;
    }

    xmaxscroll = 1 << (1 + csd->fmode_bpl);
    xdelay = (x + bm->diwstartx - csd->startx) & (fmodewidth - 1);
    xscroll = -x;

    yend = y + (bm->displayheight >> bm->interlace);
    yend = limitheight(csd, yend, FALSE, TRUE);
    ystart = y - c2d->extralines;
    if (y >= (REZ_Y_MIN + REZ_PAL_LINES) &&
        ystart < (REZ_Y_MIN + REZ_PAL_LINES))
    {
        ystart = REZ_Y_MIN + REZ_PAL_LINES;
    }

    modulo = (bm->interlace ? bm->bytesperrow : 0) + bm->modulo;
    ddfstrt = bm->ddfstrt;

    offset = ((xscroll + (xmaxscroll << 3) - 1) >> 3) & ~(xmaxscroll - 1);
    offset -= bitmap_row_offset(yscroll, bm->bytesperrow) << (bm->interlace ? 1 : 0);

    minearly = 1 << fetchunits[csd->fmode_bpl * 4 + bm->res];
    if (x & (fmodewidth - 1)) {
        ddfstrt -= minearly;
        modulo -= (minearly << bm->res) / 4;
        offset -= (minearly << bm->res) / 4;
    }

    // DIWSTRT
    copptr[1] = (y << 8) | bm->diwstartx;
    // DIWSTOP
    copptr = c2d->copper2_diwstop;
    copptr[1] = (yend << 8) |
                 ((bm->diwstartx + (bm->displaywidth >> bm->res)) & 0xff);

    /* update the bitmaps copper plane-pointers */
    setcopperplanes(csd, bm, c2d, offset, odd);

    xdelay <<= 2 - bm->res;
    // BPLCON1
    copptr[7] =
          (((xdelay >> 2) & 0x0f) << 0) | (((xdelay >> 2) & 0x0f) << 4)
        | ((xdelay >> 6) << 10) | ((xdelay >> 6) << 14)
        | ((xdelay & 3) << 8) | ((xdelay & 3) << 12);

    // DDFSTRT
    copptr[3] = ddfstrt;
    // DDFSTOP
    copptr[5] = bm->ddfstop;
    // Modulo (odd)
    copptr[9] = modulo;
    // Modulo (even)
    copptr[11] = modulo;

    // DIWHIGH
    if (csd->ecs_denise) {
        if ((copptr = c2d->copper2_fmode) != NULL)
        {
            copptr[-2] = ((y >> 8) & 7) | (((yend >> 8) & 7) << 8) | 0x2000;
        }
        else
        {
            copptr = (APTR)((IPTR)c2 + (bm->bmcl->Count << 2));
            copptr[-1] = ((y >> 8) & 7) | (((yend >> 8) & 7) << 8) | 0x2000;
        }
    }

    /* adjust start wait */
    copptr = c2;
    if (ystart >= (REZ_Y_MIN + REZ_PAL_LINES))
        copptr[0] = 0xffdf;
    else
        copptr[0] = 0x01fe;  // NOP
    copptr[2] = (ystart << 8) | 0x05;

    copptr = c2d->copper2_tail;
    if (copptr[0] != 0x0084)
    {
        /* adjust the screen end wait's ... */
        yend = y + (bm->displayheight >> bm->interlace) + yscroll;
        yend = limitheight(csd, yend, FALSE, TRUE);
        copptr[2] = (yend << 8) | 0x05;
        if (yend < (REZ_Y_MIN + REZ_PAL_LINES) || ystart >= (REZ_Y_MIN + REZ_PAL_LINES)) {
            copptr[0] = 0x00df;
            copptr[1] = 0x00fe;
        } else {
            copptr[0] = 0xffdf;
            copptr[1] = 0xfffe;
        }
    }
}

static VOID setcopperscrollbm(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    setcopperscroll(csd, bm, ((csd->interlaced == TRUE) || (bm->interlace == TRUE)));
}

VOID setcopperscroll(struct amigavideo_staticdata *csd, struct amigabm_data *bm, BOOL interlaced)
{
    if (bm->bmcl)
    {
        setcopperscroll2(csd, bm, &bm->copld, bm->bmcl->CopLStart, FALSE);
        if (interlaced)
            setcopperscroll2(csd, bm, &bm->copsd, bm->bmcl->CopSStart, bm->interlace);
    }
}

UWORD get_copper_list_length(struct amigavideo_staticdata *csd, UBYTE depth)
{
    UWORD v;

    if (csd->aga && csd->aga_enabled) {
        v = 1000 + ((1 << depth) + 1 + (1 << depth) / 32 + 1) * 2;
    } else {
        v = 1000;
    }
    return v * 2;
}

VOID updatebmbplcon(struct amigavideo_staticdata *csd, struct amigabm_data *bm, struct copper2data *c2d)
{
    UWORD *c;
    UWORD bplcon3;

    D(bug("[AmigaVideo] %s()\n", __func__));

    bplcon3 = csd->bplcon3 | bm->bplcon3 | ((bm->sprite_res + 1) << 6);

    // update AGA only palette values ...
#if (0)
    if (csd->aga && csd->aga_enabled) {
        c = c2d->copper2_palette;
        c[1] = bm->bplcon3 | ((i / 32) << 13);
        c2d->copper2_palette_aga_lo = c;
        c[1] = bm->bplcon3 | ((i / 32) << 13) | 0x0200;
        c = (UWORD *)((IPTR)c2d->copper2_palette_aga_lo + ((bm->use_colors + 1) << 2));
        c[1] = bm->bplcon3;
    }
#endif

    c = c2d->copper2_scroll;
    if (csd->aga && csd->aga_enabled) {
        /* update (BPLCON4) bitplane display masks  */
        c[-1] = csd->bplcon4;
        D(bug("[AmigaVideo] %s: bm bplcon4 = %04x\n", __func__, c[-1]));
    }

    c[5] = csd->bplcon2 | ((csd->aga && !(bm->modeid & EXTRAHALFBRITE_KEY)) ? 0x0200 : 0);
    c[7] = csd->bplcon3;
    D(
      bug("[AmigaVideo] %s: bm bplcon3 = %04x\n", __func__, c[7]);
      bug("[AmigaVideo] %s: bm bplcon2 = %04x\n", __func__, c[5]);
     )
}

UWORD *populatebmcopperlist(struct amigavideo_staticdata *csd, struct amigabm_data *bm, struct copper2data *c2d, UWORD *c2, BOOL lace)
{
    struct GfxBase *GfxBase = (struct GfxBase *)csd->cs_GfxBase;
    volatile UWORD *system_bplcon0 = (volatile UWORD *)&GfxBase->system_bplcon0;
    UWORD *c = c2;
    UWORD i;
    UWORD bplcon0, bplcon0_res, bplcon0_null, bplcon3;
    UWORD palette_bplcon3;
    ULONG pptr;

    D(bug("[AmigaVideo] %s()\n", __func__));

    D(bug("[AmigaVideo] %s: Copperlist%d @ 0x%p\n", __func__,  lace ? 2 : 1, c));

    bplcon0_res = *system_bplcon0 & ~BPLCONMASK;
    D(bug("[AmigaVideo] %s: initial bplcon0 = %04x\n", __func__, bplcon0_res));

    if (bm->res == 1)
         bplcon0_res |= 0x8000;
    else if (bm->res == 2)
        bplcon0_res |= 0x0040;
    else
        bplcon0_res = 0;

    bplcon0_null = csd->bplcon0_null | (bm->interlace ? 4 : 0) | bplcon0_res;
    palette_bplcon3 = csd->bplcon3 | bm->bplcon3;
    bplcon3 = csd->bplcon3 | bm->bplcon3 | ((bm->sprite_res + 1) << 6);

    D(bug("[AmigaVideo] %s: bplcon0_null = %04x\n", __func__, bplcon0_null));
    D(bug("[AmigaVideo] %s: bm bplcon3 = %04x\n", __func__, bplcon3));

    /* NOP + WAIT - to store start co-ords */
    COPPEROUT(c, 0x01fe, 0xfffe)
    COPPEROUT(c, 0xffff, 0xfffe)

    bm->use_colors = 1 << bm->depth;

    // need to update sprite colors
    if (bm->use_colors < 16 + 4)
        bm->use_colors = 16 + 4;
    if (bm->res == 2 && !csd->aga)
        bm->use_colors = 32; /* ECS "scrambled" superhires */
    
    if (bm->use_colors > 32 && (bm->modeid & EXTRAHALFBRITE_KEY))
        bm->use_colors = 32;
    if (bm->modeid & HAM_KEY) {
        if (bm->depth <= 6)
            bm->use_colors = 16 + 4;
        else
            bm->use_colors = 64;
    }

    // Push the palette registers ...
    c2d->copper2_palette = c;
    if (csd->aga && csd->aga_enabled) {
        // hi
        for (i = 0; i < bm->use_colors; i++) {
            UBYTE agac = i & 31;
            if (agac == 0) {
                COPPEROUT(c, 0x0106, palette_bplcon3 | ((i / 32) << 13))
            }
            COPPEROUT(c, 0x0180 + agac * 2, 0x0000)
        }
        c2d->copper2_palette_aga_lo = c;
        // lo
        for (i = 0; i < bm->use_colors; i++) {
            UBYTE agac = i & 31;
            if (agac == 0) {
                COPPEROUT(c, 0x0106, palette_bplcon3 | ((i / 32) << 13) | 0x0200)
            }
            COPPEROUT(c, 0x0180 + agac * 2, 0x000)
        }
        COPPEROUT(c, 0x106, palette_bplcon3)
    } else {
        // ocs/ecs
        for (i = 0; i < bm->use_colors; i++) {
            COPPEROUT(c, 0x180 + i * 2, 0x000)
        }
    }

    if (csd->aga && csd->aga_enabled) {
        /* Push (BPLCON4) bitplane display masks  */
        COPPEROUT(c, 0x010c, csd->bplcon4)
    }

    bplcon0 = bplcon0_null;
    if (bm->depth > 7)
        bplcon0 |= 0x0010;
    else
        bplcon0 |= bm->depth << 12;
    if (bm->modeid & HAM_KEY)
        bplcon0 |= 0x0800;
    D(bug("[AmigaVideo] %s: copper bplcon0 = %04x\n", __func__, bplcon0));

    c2d->copper2_scroll = c;

    COPPEROUT(c, 0x008e, 0)                     // Push (DIWSTRT) Display window start

    COPPEROUT(c, 0x0100, bplcon0)               // Push the screens bplcon0
    COPPEROUT(c, 0x0104, csd->bplcon2 | ((csd->aga && !(bm->modeid & EXTRAHALFBRITE_KEY)) ? 0x0200 : 0))  // Push the screens bplcon2
    if (csd->ecs_denise) {
        COPPEROUT(c, 0x0106, bplcon3)           // Push the screens bplcon3
    }

    c2d->copper2_diwstop = c;
    COPPEROUT(c, 0x0090, 0)                     // Push (DIWSTOP) Display window stop
    COPPEROUT(c, 0x0092, 0)                     // Push (DDFSTRT) Display bitplane data fetch start
    COPPEROUT(c, 0x0094, 0)                     // Push (DDFSTOP) Display bitplane data fetch stop
    COPPEROUT(c, 0x0102, 0)                     // Push (BPLCON1) Bitplane scroll control reg
    COPPEROUT(c, 0x0108, 0)                     // Push Bitplane modulo (odd planes)
    COPPEROUT(c, 0x010a, 0)                     // Push Bitplane modulo (even planes)

    c2d->copper2_bpl = c;
    for (i = 0; i < bm->depth; i++) {
        pptr = (ULONG)(bm->pbm->Planes[bm->bploffsets[i]]);
        if (lace)
            pptr += bm->bytesperrow;
        COPPEROUT(c, 0x00e0 + i * 4, (UWORD)(pptr >> 16)) // Push the bitplane registers
        COPPEROUT(c, 0x00e2 + i * 4, (UWORD)(pptr >> 0))
    }

    if (csd->ecs_denise) {
        COPPEROUT(c, 0x01e4, 0)                 // Push (DIWHIGH) Display window
    }
    c2d->copper2_fmode = NULL;
    if (csd->aga && csd->aga_enabled) {
        COPPEROUT(c, 0x01fc, 0)
        c2d->copper2_fmode = &c[-1];
    }

    c2d->extralines = (((IPTR)c - (IPTR)c2) / 112) + 1;

    /* store the pointer to the copperlist data tail so it can be adjusted for linking chains */
    c2d->copper2_tail = c;

    return c;
}

VOID setcopperlisttail(struct amigavideo_staticdata *csd, UWORD *copper2tail, UWORD *c2next, BOOL jmp)
{
    UWORD *c = copper2tail;

    D(bug("[AmigaVideo] %s(0x%p)\n", __func__, c2next));
    D(bug("[AmigaVideo] %s: tail @ 0x%p\n", __func__, c));

    if (!jmp)
    {
        COPPEROUT(c, 0xffff, 0xfffe)
        COPPEROUT(c, 0xffff, 0xfffe)
    }
    if (c2next)
    {
        COPPEROUT(c, 0x0084, (UWORD)((IPTR)c2next >> 16))
        COPPEROUT(c, 0x0086, (UWORD)((IPTR)c2next >> 0))
        if (jmp)
        {
            D(bug("[AmigaVideo] %s: pushing COPJMP2\n", __func__));
            COPPEROUT(c, 0x008A, 0x0000)
        }
    }
    COPPEROUT(c, 0xffff, 0xfffe)                // END
}

BOOL setbitmap(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    D(bug("[AmigaVideo] %s()\n", __func__));

    bm->modulo = bm->bytesperrow - bm->modulopre / (4 >> bm->res);
    bm->modulo &= ~((2 << csd->fmode_bpl) - 1);

    if (bm->bmcl && bm->bmcl->CopLStart)
        setcopperscrollbm(csd, bm);

    D(bug("[AmigaVideo] %s: bm=%x mode=%08x w=%d h=%d d=%d bpr=%d\n",
        __func__, bm, bm->modeid, bm->width, bm->height, bm->depth, bm->bytesperrow));
        return TRUE;
}

BOOL setmode(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;
    UWORD *c;
    UWORD ddfstrt, ddfstop;
    UBYTE fetchunit, maxplanes;
    UWORD fetchpixels, fetchspan, viewwidth;
    UBYTE i;
    BYTE bestmode;

    D(bug("[AmigaVideo] %s(0x%p)\n", __func__, bm));

    csd->fmode_bpl = 0;
    if (csd->aga && csd->aga_enabled)
    {
        bestmode = bestbplfetchmode(bm);
        if (bestmode < 0)
            return FALSE;
        csd->fmode_bpl = bestmode;
    }

    fetchunit = fetchunits[csd->fmode_bpl * 4 + bm->res];
    maxplanes = fm_maxplanes[csd->fmode_bpl * 4 + bm->res];

    D(bug("[AmigaVideo] %s: res %d fmode %d depth %d maxplanes %d aga %d agae %d\n",
        __func__, bm->res, csd->fmode_bpl, bm->depth, maxplanes, csd->aga, csd->aga_enabled));

    if (bm->depth > (1 << maxplanes)) {
        if (csd->aga && !csd->aga_enabled) {
            // Enable AGA if requesting AGA only mode.
            // This is a compatibility hack because our display
            // database currently contains all AGA modes even if AGA
            // is "disabled".
            GfxBase->ChipRevBits0 = SETCHIPREV_AA | GFXF_AA_MLISA;
            csd->aga_enabled = TRUE;
            bestmode = bestbplfetchmode(bm);
            if (bestmode < 0)
                return FALSE;
            csd->fmode_bpl = bestmode;
            fetchunit = fetchunits[csd->fmode_bpl * 4 + bm->res];
            maxplanes = fm_maxplanes[csd->fmode_bpl * 4 + bm->res];
        }
        if (bm->depth > (1 << maxplanes))
            return FALSE;
    }

    viewwidth = bm->width;
    // use nominal width for now
    if ((viewwidth << bm->res) > 320)
        viewwidth = 320 << bm->res;

    D(bug("[AmigaVideo] %s:  mode %08x (%dx%dx%d) bpr=%d fu=%d\n",
        __func__, bm->modeid, bm->width, bm->height, bm->depth, bm->bytesperrow, fetchunit));

    fetchspan = 1 << fetchunit;
    fetchpixels = fetchspan << (bm->res + 1);
    ddfstrt = (bm->diwstartx / 2) & ~(fetchspan - 1);
    ddfstrt -= 1 << maxplanes;
    /*
     * DDFSTOP names the start of the last fetch unit, so it sits one unit
     * short of the span's end: a 640 pixel hires line in FMODE 0 is 20 units
     * of 8 colour clocks from $3c and stops at $d4, as Kickstart programs it.
     * Placing it a unit further ($dc) is beyond the $d8 the hardware stops
     * at, so that unit is never fetched while the modulo below still pays
     * for it, shearing the display.
     */
    ddfstop = ddfstrt +
        (((viewwidth + fetchpixels - 1) / fetchpixels) - 1) * fetchspan;

    /*
     * The fetched span includes the unit beginning at DDFSTOP.  Keep
     * modulopre in colour-clock units for setbitmap(), which converts it
     * back to bytes according to display resolution.
     */
    bm->modulopre = ddfstop - ddfstrt + fetchspan;
    bm->ddfstrt = ddfstrt;
    bm->ddfstop = ddfstop;

    if ((bm->modeid & HAM_KEY) && bm->depth > 6) {
        bm->bploffsets[0] = 6;
        bm->bploffsets[1] = 7;
        for (i = 0; i < 6; i++)
            bm->bploffsets[i + 2] = i;
    }
    else
    {
        for (i = 0; i < 8; i++)
            bm->bploffsets[i] = i;
    }

    setfmode(csd, bm);
    bm->displaywidth = viewwidth;

    setbitmap(csd, bm);

    setspritepos(csd, csd->spritex, csd->spritey, bm->res, bm->interlace);
    
    for(i=1;i<8;i++)
      new_setspritepos(csd, csd->new_spritex[i], csd->new_spritey[i], bm->res, bm->interlace, i);

    D(bug("[AmigaVideo] %s: done\n", __func__));

    return 1;
 }

UBYTE av__PickPen(struct amigavideo_staticdata *csd, ULONG pixel)
{
    UBYTE retval = 1;
    return retval;
}

/* Convert a pointer or sprite shape to hardware sprite data. cursor selects
   the mouse pointer's state, otherwise sprite spritenum's. */
static BOOL setspriteshape(OOP_Class *cl, OOP_Object *o, WORD width, WORD height,
                           OOP_Object *shape, WORD xoffset, WORD yoffset,
                           BOOL cursor, int spritenum)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *OOPBase = csd->cs_OOPBase;
    OOP_MethodID HiddGfxBase = csd->cs_HiddGfxBase;
    OOP_MethodID HiddBitMapBase = csd->cs_HiddBitMapBase;
    OOP_MethodID HiddDMEnumBase = csd->cs_HiddDMEnumBase;
    struct amigabm_data *data = OOP_INST_DATA(cl, o);
    OOP_Object *bmPFObj = NULL;
    HIDDT_PixelFormat *bmPF;
    IPTR pf, bmcmod, bytesperpixel = 0;
    BOOL indexed8 = FALSE;
    UWORD fetchsize;
    UWORD bitmapwidth = width;
    UWORD y, *p;

    D(bug("[AmigaVideo] %s()\n", __func__));

    OOP_GetAttr(shape, aHidd_BitMap_PixFmt, (IPTR*)&bmPFObj);
    OOP_GetAttr(bmPFObj, aHidd_PixFmt_ColorModel, &bmcmod);
    if (cursor)
    {
        OOP_GetAttr(bmPFObj, aHidd_PixFmt_BytesPerPixel, &bytesperpixel);
        indexed8 = bmcmod != vHidd_ColorModel_TrueColor && bytesperpixel == 1;
    }
    if (bmcmod == vHidd_ColorModel_TrueColor)
    {
        OOP_GetAttr(bmPFObj, aHidd_PixFmt_StdPixFmt, (IPTR*)&pf);
        bmPF = (HIDDT_PixelFormat *)HIDD_DMEnum_GetPixFmt(csd->dmenum, pf);
    }

    if (csd->aga && csd->aga_enabled && width > 16)
        csd->fmode_spr = 2;
    else
        csd->fmode_spr = 0;
    D(bug("[AmigaVideo] %s: fmode_spr = %x\n", __func__, csd->fmode_spr));
    fetchsize = 2 << csd->fmode_spr;
    width = 16 << csd->fmode_spr;

    if (cursor)
    {
        if (width != csd->sprite_width || height != csd->sprite_height) {
            resetsprite(csd);
            csd->spritedatasize = fetchsize * 2 + fetchsize * height * 2 + fetchsize * 2;
            csd->sprite = AllocMem(csd->spritedatasize, MEMF_CHIP | MEMF_CLEAR);
            if (!csd->sprite)
                return FALSE;
            csd->sprite_width = width;
            csd->sprite_height = height;
        }
        /* Pointer shapes of the same dimensions can have different hot spots. */
        csd->sprite_offset_x = xoffset;
        csd->sprite_offset_y = yoffset;
        p = csd->sprite;
    }
    else
    {
        if (width != csd->new_sprite_width[spritenum] || height != csd->new_sprite_height[spritenum]) {
            new_resetsprite(csd, spritenum);
            csd->new_spritedatasize[spritenum] = fetchsize * 2 + fetchsize * height * 2 + fetchsize * 2;
            csd->new_sprite[spritenum] = AllocMem(csd->new_spritedatasize[spritenum], MEMF_CHIP | MEMF_CLEAR);
            if (!csd->new_sprite[spritenum])
                return FALSE;
            csd->new_sprite_width[spritenum] = width;
            csd->new_sprite_height[spritenum] = height;
            csd->new_sprite_offset_x[spritenum] = xoffset;
            csd->new_sprite_offset_y[spritenum] = yoffset;
        }
        p = csd->new_sprite[spritenum];
    }
    p += fetchsize;
    for(y = 0; y < height; y++) {
        UBYTE rowpixels[64];
        UWORD xx, xxx, x;

        if (indexed8)
            HIDD_BM_GetImage(shape, rowpixels, sizeof(rowpixels),
                             0, y, bitmapwidth < width ? bitmapwidth : width,
                             1, vHidd_StdPixFmt_Native);

        for (xx = 0, xxx = 0; xx < width; xx += 16, xxx++) {
            UWORD pix1 = 0, pix2 = 0;
            for(x = 0; x < 16; x++) {
                UBYTE c = 0;
                if (xx + x < bitmapwidth)
                {
                    if (bmcmod != vHidd_ColorModel_TrueColor)
                        c = indexed8 ? rowpixels[xx + x] :
                            HIDD_BM_GetPixel(shape, xx + x, y);
                    else
                    {
                        HIDDT_Pixel pix = HIDD_BM_GetPixel(shape, xx + x, y);
                        c = 0;
                        if ((ALPHA_COMP(pix, bmPF) & 0xFF00) == 0xFF00)
                            c = av__PickPen(csd, ((RED_COMP(pix, bmPF) & 0xFF00) << 8) | (GREEN_COMP(pix, bmPF) & 0xFF00) | ((BLUE_COMP(pix, bmPF) >> 8) & 0xFF));
                        else c = 0;
                    }
                }
                pix1 <<= 1;
                pix2 <<= 1;
                pix1 |= (c & 1) ? 1 : 0;
                pix2 |= (c & 2) ? 1 : 0;
            }
            p[xxx] = pix1;
            p[xxx + fetchsize / 2] = pix2;
        }
        p += fetchsize;
    }
    if (cursor)
    {
        setspritepos(csd, csd->spritex, csd->spritey, data->res, data->interlace);
        setspritevisible(csd, csd->cursorvisible);
    }
    else
    {
        new_setspritepos(csd, csd->spritex, csd->spritey, data->res, data->interlace, spritenum);
        new_setspritevisible(csd, csd->spritevisible[spritenum], spritenum);
    }
    return TRUE;
}

BOOL setsprite(OOP_Class *cl, OOP_Object *o, WORD width, WORD height, struct pHidd_Display_SetCursorShape *msg)
{
    return setspriteshape(cl, o, width, height, msg->shape, msg->xoffset, msg->yoffset, TRUE, 0);
}

BOOL new_setsprite(OOP_Class *cl, OOP_Object *o, WORD width, WORD height, struct pHidd_AmigaGfx_SetSpriteShape *msg, int spritenum)
{
    return setspriteshape(cl, o, width, height, msg->shape, msg->xoffset, msg->yoffset, FALSE, spritenum);
}

VOID setspritepos(struct amigavideo_staticdata *csd, WORD x, WORD y, UBYTE res, BOOL interlace)
{
    UWORD ctl, pos;
    WORD height = csd->classic_cursor_data ? csd->classic_cursor_height : csd->sprite_height;
    WORD xoffset = csd->classic_cursor_data ? csd->classic_cursor_xoffset : csd->sprite_offset_x;
    WORD yoffset = csd->classic_cursor_data ? csd->classic_cursor_yoffset : csd->sprite_offset_y;

    csd->spritex = x;
    csd->spritey = y;
    if ((!csd->sprite && !csd->classic_cursor_data) || height == 0)
        return;

    x += xoffset << res;
    x <<= (2 - res); // convert x to shres coordinates
    x += csd->startx << 2; // sprite origin is not the bitplane fetch offset
 
    if (interlace)
        y >>= 1; // y is always in nonlaced
    y += csd->starty;
    y += yoffset;

    pos = (y << 8) | (x >> 3);
    ctl = ((y + height) << 8);
    ctl |= ((y >> 8) << 2) | (((y + height) >> 8) << 1) | ((x >> 2) & 1) | ((x & 3) << 3);
    csd->spritepos = pos;
    csd->spritectl = ctl;
    if (csd->classic_cursor_data)
    {
        csd->classic_cursor_data[0] = pos;
        csd->classic_cursor_data[1] = ctl;
    }
}

VOID new_setspritepos(struct amigavideo_staticdata *csd, WORD x, WORD y, UBYTE res, BOOL interlace, int spritenum)
{
    UWORD ctl, pos;
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    csd->new_spritex[spritenum] = x;
    csd->new_spritey[spritenum] = y;
    if (!csd->new_sprite[spritenum] || csd->new_sprite_height[spritenum] == 0)
        return;

    x += csd->new_sprite_offset_x[spritenum] << res;
    x <<= (2 - res); // convert x to shres coordinates
    x += (csd->startx - 1) << 2; // display left edge offset

    if (interlace)
        y >>= 1; // y is always in nonlaced
    y += csd->starty;
    y += csd->new_sprite_offset_y[spritenum];

    pos = (y << 8) | (x >> 3);
    ctl = ((y + csd->new_sprite_height[spritenum]) << 8);
    ctl |= ((y >> 8) << 2) | (((y + csd->new_sprite_height[spritenum]) >> 8) << 1) | ((x >> 2) & 1) | ((x & 3) << 3);
    csd->new_spritepos[spritenum] = pos;
    csd->new_spritectl[spritenum] = ctl;
    custom->spr[spritenum].pos = pos;
    custom->spr[spritenum].ctl = ctl;
}

VOID setspritevisible(struct amigavideo_staticdata *csd, BOOL visible)
{
    D(bug("[AmigaVideo] %s()\n", __func__));

    csd->cursorvisible = visible;
    pointsprite(csd, csd->copper1_spritept,
                csd->classic_cursor_data ? csd->classic_cursor_data : csd->sprite,
                csd->spritey, visible);
}

VOID new_setspritevisible(struct amigavideo_staticdata *csd, BOOL visible, int spritenum)
{
    D(bug("[AmigaVideo] %s()\n", __func__));
    
    csd->spritevisible[spritenum] = visible;
    pointsprite(csd, csd->new_copper1_spritept[spritenum], csd->new_sprite[spritenum],
                csd->new_spritey[spritenum], visible);
}

BOOL setcolors(struct amigavideo_staticdata *csd, struct amigabm_data *bm, struct pHidd_BitMap_SetColors *msg)
{
    D(bug("[AmigaVideo] %s()\n", __func__));

    if (!(bm->palette))
    {
        D(bug("[AmigaVideo] %s: missing palette data!\n", __func__));
        return FALSE;
    }

    if (msg->firstColor + msg->numColors > csd->max_colors)
    {
        D(bug("[AmigaVideo] %s: too many colors specified!\n", __func__));
        return FALSE;
    }

    UWORD i, j = 0;

    Disable();
    for (i = msg->firstColor; j < msg->numColors; i++, j++) {
        UBYTE red, green, blue;
        red   = msg->colors[j].red   >> 8;
        green = msg->colors[j].green >> 8;
        blue  = msg->colors[j].blue  >> 8;
        bm->palette[i * 3 + 0] = red;
        bm->palette[i * 3 + 1] = green;
        bm->palette[i * 3 + 2] = blue;
        //bug("%d: %02x %02x %02x\n", i, red, green, blue);
    }

    /*
     * CopLStart belongs to the preliminary DspIns and may be detached after
     * MrgCop(), while copld continues to point at the active merged list.
     * Late SetRGB*() calls must therefore key off the live palette pointer,
     * not the lifetime of the preliminary list storage.
     */
    if (bm->copld.copper2_palette && bm->vis)
        bm->palette_changed = TRUE;
    Enable();

    /* A detached bitmap has no live copper reader and can be updated now.
     * Visible palettes are committed by ChangeVPBitMap(), or by VBlank when
     * the application performs a palette-only change. */
    if (bm->copld.copper2_palette && !bm->vis)
        setcoppercolors(csd, bm, bm->palette, FALSE);

    return TRUE;
}

VOID setscroll(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    csd->updatescroll = bm;
}

/* Convert Z flag to normal C-style return variable. Fun. */
UBYTE bltnode_wrapper(VOID)
{
    UBYTE ret;
    asm volatile (
       "    pea 1f\n"
       "    move.l 4(%%a1),-(%%sp)\n"
       "    rts\n"
       "1:  sne %d0\n"
       "    move.b %%d0,%0\n"
       : "=g" (ret)
    );
    return ret;
}

#define BEAMSYNC_ALARM 0x0f00
/* AOS must use some GfxBase flags field for these. Later.. */
#define bqvar GfxBase->pad3
#define BQ_NEXT 1
#define BQ_BEAMSYNC 2
#define BQ_BEAMSYNCWAITING 4
#define BQ_MISSED 8

static AROS_INTH1(gfx_blit, struct GfxBase *, GfxBase)
{
    AROS_INTFUNC_INIT

    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct bltnode *bn = NULL;
    UBYTE v;
    UWORD dmaconr;
    
    dmaconr = custom->dmaconr;
    dmaconr = custom->dmaconr;
    if (dmaconr & 0x4000) {
        /* Blitter still active? Wait for next interrupt. */
        return 0;
    }
    
    if (GfxBase->blthd == NULL && GfxBase->bsblthd == NULL) {
        custom->intena = INTF_BLIT;
        return 0;
    }
    
    /* Was last blit in this node? */
    if (bqvar & BQ_NEXT) {
        bqvar &= ~(BQ_NEXT | BQ_MISSED);
        if (bqvar & BQ_BEAMSYNC)
            bn = GfxBase->bsblthd;
        else
            bn = GfxBase->blthd;
        if (bn->stat == CLEANUP)
            AROS_UFC2(UBYTE, bn->cleanup,
                AROS_UFCA(struct Custom *, custom, A0),
                AROS_UFCA(struct bltnode*, bn, A1));
        /* Next node */
        bn = bn->n;
        if (bqvar & BQ_BEAMSYNC)
            GfxBase->bsblthd = bn;
        else
            GfxBase->blthd = bn;
    }

    if (GfxBase->bsblthd) {
        bn = GfxBase->bsblthd;
        bqvar |= BQ_BEAMSYNC;
    } else if (GfxBase->blthd) {
        bn = GfxBase->blthd;
        bqvar &= ~BQ_BEAMSYNC;
    }

    if (!bn) {
        /* Last blit finished */
        bqvar = 0;
        custom->intena = INTF_BLIT;
        GfxBase->blthd = GfxBase->bsblthd = NULL;
        DisownBlitter();
        return 0;
    }

    if (bqvar & BQ_BEAMSYNC) {
        UWORD vpos = VBeamPos();
        bqvar &= ~BQ_BEAMSYNCWAITING;
        if (!(bqvar & BQ_MISSED) && bn->beamsync > vpos) {
            volatile struct CIA *ciab = (struct CIA*)0xbfd000;
            UWORD w = BEAMSYNC_ALARM - (bn->beamsync - vpos);
            bqvar |= BQ_BEAMSYNCWAITING;
            ciab->ciacrb &= ~0x80;
            ciab->ciatodhi = 0;
            ciab->ciatodmid = w >> 8;
            ciab->ciatodlow = w;
            return 0;
        }
    }
 
    v = AROS_UFC2(UBYTE, bltnode_wrapper,
            AROS_UFCA(struct Custom *, custom, A0),
            AROS_UFCA(struct bltnode*, bn, A1));

    dmaconr = custom->dmaconr;
    dmaconr = custom->dmaconr;
    if (!(dmaconr & 0x4000)) {
        /* Eh? Blitter not active?, better fake the interrupt. */
        custom->intreq = INTF_SETCLR | INTF_BLIT;
    }
    
    if (v) {
        /* Handle same node again next time */
        return 0;
    }

    bqvar |= BQ_NEXT;

    return 0;
        
    AROS_INTFUNC_EXIT
}

static AROS_INTH1(gfx_beamsync, struct amigavideo_staticdata*, csd)
{
    AROS_INTFUNC_INIT

    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;

    if (bqvar & BQ_BEAMSYNCWAITING) {
        /* We only need to trigger blitter interrupt */
        volatile struct Custom *custom = (struct Custom*)0xdff000;
        custom->intreq = INTF_SETCLR | INTF_BLIT;
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

/* the BPLCON words of copper1 follow the topmost screen */
static void updatecopper1bplcon(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    csd->copper1[5] = csd->bplcon3 | bm->bplcon3 | ((bm->sprite_res + 1) << 6);
    csd->copper1[9] = csd->bplcon3 | bm->bplcon3 | (1 << 9) | ((bm->sprite_res + 1) << 6);
    csd->copper1[13] = csd->bplcon3 | bm->bplcon3 | ((bm->sprite_res + 1) << 6);
    csd->copper1[27] = csd->bplcon2 | ((csd->aga && !(bm->modeid & EXTRAHALFBRITE_KEY)) ? 0x0200 : 0);
}

/* update the copper1 data to reflect the topmost screen */
VOID updatecopper1top(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    updatecopper1bplcon(csd, bm);
    if (csd->aga && csd->aga_enabled)
    {
        csd->copper1[7] = bm->copld.copper2_palette[3];
        csd->copper1[11] = bm->copld.copper2_palette[3];
    }
    else
    {
        csd->copper1[7] = bm->copld.copper2_palette[1];
        csd->copper1[11] = bm->copld.copper2_palette[1];
    }
}

/* update the copper1 data to reflect the requested bm */
static void updatecopper1frombm(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    D(bug("[AmigaVideo] %s(0x%p)\n", __func__, bm));

    updatecopper1bplcon(csd, bm);
    if (bm->copld.copper2_palette)
    {
        if (csd->aga_enabled)
        {
            csd->copper1[7] = bm->copld.copper2_palette[3];
            csd->copper1[11] = bm->copld.copper2_palette[3];
        }
        else
        {
            csd->copper1[7] = bm->copld.copper2_palette[1];
            csd->copper1[11] = bm->copld.copper2_palette[1];
        }
    }
    else
    {
        csd->copper1[7] = DEFAULT_BORDER_GRAY;
        csd->copper1[11] = DEFAULT_BORDER_GRAY;
    }
    if (bm->copld.copper2_fmode)
        csd->copper1[15] = *bm->copld.copper2_fmode;
    else
        csd->copper1[15] = 0;
}

/* attach a hidden screen to the visible copperlist chains */
static BOOL gfx_vblank_attachbm(struct amigavideo_staticdata *csd, struct amigabm_data *bm, struct amigabm_data *bmpred)
{
    struct HIDD_ViewPortData *vpdata = (struct HIDD_ViewPortData *)bm->node.ln_Name;
    struct ViewPort  *bmvp = vpdata->vpe->ViewPort;
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;
    struct amigabm_data *bmtmp;
    LONG screen_start, screen_finish = (STANDARD_DENISE_MAX << 1);

    D(bug("[AmigaVideo] %s(0x%p)\n", __func__, bm));

    if (!csd->interlaced && bm->interlace)
    {
        /*
        Signal the support task to force a rethink of the display
        */
        D(bug("[AmigaVideo] %s: display rebuild needed\n", __func__));
        if (csd->svcTask)
            Signal(csd->svcTask, SIGBREAKF_CTRL_F);
        return FALSE;
    }

    /* calculate and adjust the screens visible region */
    screen_start = bm->topedge;
    if (!bmpred || (((bmtmp = (struct amigabm_data *)bmpred->node.ln_Succ) == NULL) ||
        (bmtmp->node.ln_Succ == NULL)))
    {
        bmtmp = (struct amigabm_data *)GetHead(csd->compositedbms);
    }

    if (bmtmp)
    {
        screen_finish = bmtmp->topedge - (bmtmp->copld.extralines + 1);
        if ((bm->interlace) && (!bmtmp->interlace))
            screen_finish <<= 1;
        else if ((!bm->interlace) && (bmtmp->interlace))
            screen_finish >>= 1;
    }
    else
    {
        /* screen_finish is an absolute display coordinate. */
        screen_finish = bm->topedge + bm->viewportheight - 1;
    }
    setdisplayheight(csd, bm, (screen_finish - screen_start) + 1);
    D(bug("[AmigaVideo] %s: screen range = %d -> %d (%d rows)\n", __func__, screen_start, screen_finish, bm->displayheight);)
    /* sanity check .. */
    if (bm->displayheight <= 1)
    {
        /*
         * either not enough of the screen is visible,
         * or it is obscured by the next screens copperlist pre-amble
         */
        return FALSE;
    }

    if (bmpred)
        Insert(&csd->c2fragments, &bm->copld.cnode, &bmpred->copld.cnode);
    else
        AddHead(&csd->c2fragments, &bm->copld.cnode);

    if (bm->interlace)
    {
        if (bmpred)
            Insert(&csd->c2ifragments, &bm->copsd.cnode, &bmpred->copsd.cnode);
        else
            AddHead(&csd->c2ifragments, &bm->copsd.cnode);
    }
    else if (csd->interlaced)
    {
        initvpicopper(bmvp,bm);

        if (bmpred)
            Insert(&csd->c2ifragments, &bm->copsd.cnode, &bmpred->copsd.cnode);
        else
            AddHead(&csd->c2ifragments, &bm->copsd.cnode);
    }

    D(
        if (!bmpred)
            bug("[AmigaVideo] %s: * new topmost bitmap\n", __func__);
      )

    setcopperscrollbm(csd, bm);

    if (bm->bmucl)
    {
        AmigaVideo_ParseCopperlist(csd, bm, bmvp->UCopIns->FirstCopList);
#if !USE_UCOP_DIRECT
        D(bug("[AmigaVideo] %s:  -- copying user-copperlist data ...\n", __func__);)
        CopyMemQuick(bm->bmucl->CopLStart, bm->copld.copper2_tail, bm->bmuclsize);
        bm->copld.copper2_tail = (APTR)((IPTR)bm->copld.copper2_tail + bm->bmuclsize);
        if ((bm->interlace) || (csd->interlaced))
        {
            if (bm->bmucl->CopSStart)
                CopyMemQuick(bm->bmucl->CopSStart, bm->copsd.copper2_tail, bm->bmuclsize);
            else
                CopyMemQuick(bm->bmucl->CopLStart, bm->copsd.copper2_tail, bm->bmuclsize);
            bm->copsd.copper2_tail = (APTR)((IPTR)bm->copsd.copper2_tail + bm->bmuclsize);
        }
        bm->bmucl->Flags |= (1<<15);
#endif
    }

    /* link the copperlist fragments */
    if (bm->copld.cnode.mln_Pred && bm->copld.cnode.mln_Pred->mln_Pred)
    {
        bmtmp = BMDATFROMCOPLD(bm->copld.cnode.mln_Pred);
        setcopperlisttail(csd, bmtmp->copld.copper2_tail, bm->bmcl->CopLStart, TRUE);                                           // PREV CopL >> THIS CopL
        D(bug("[AmigaVideo] %s:     CopL @ 0x%p jmp to CopL @ 0x%p\n", __func__, &bmtmp->copld, &bm->copld);)
    }
    else
    {
        /* update the copper1 data to reflect the topmost screen */
        updatecopper1frombm(csd, bm);
    }

    if (bm->copld.cnode.mln_Succ && bm->copld.cnode.mln_Succ->mln_Succ)
    {
        bmtmp = BMDATFROMCOPLD(bm->copld.cnode.mln_Succ);
        setcopperlisttail(csd, bm->copld.copper2_tail, bmtmp->bmcl->CopLStart, TRUE);                                           // THIS CopL >> NEXT CopL
        D(bug("[AmigaVideo] %s:     CopL @ 0x%p jmp to CopL @ 0x%p\n", __func__, &bm->copld, &bmtmp->copld);)
    }

    if ((bm->interlace) || (csd->interlaced))
    {
        if (bm->copsd.cnode.mln_Pred && bm->copsd.cnode.mln_Pred->mln_Pred)
        {
            bmtmp = BMDATFROMCOPSD(bm->copsd.cnode.mln_Pred);
            setcopperlisttail(csd, bmtmp->copsd.copper2_tail, bm->bmcl->CopSStart, TRUE);                                       // PREV CopS >> THIS CopS
            D(bug("[AmigaVideo] %s:     CopS @ 0x%p jmp to CopS @ 0x%p\n", __func__, &bmtmp->copsd, &bm->copsd);)
        }
        if (bm->copsd.cnode.mln_Succ && bm->copsd.cnode.mln_Succ->mln_Succ)
        {
            bmtmp = BMDATFROMCOPSD(bm->copsd.cnode.mln_Succ);
            setcopperlisttail(csd, bm->copsd.copper2_tail, bmtmp->bmcl->CopSStart, TRUE);                                       // THIS CopS >> NEXT CopS
            D(bug("[AmigaVideo] %s:     CopS @ 0x%p jmp to CopS @ 0x%p\n", __func__, &bm->copsd, &bmtmp->copsd);)
        }
        else
        {
            bmtmp = BMDATFROMCOPLD(GetHead(&csd->c2fragments));
            setcopperlisttail(csd, bm->copsd.copper2_tail, bmtmp->bmcl->CopLStart, FALSE);                                      // THIS CopS -> FIRST CopL
            D(bug("[AmigaVideo] %s:     CopS @ 0x%p point to CopL @ 0x%p\n", __func__, &bm->copsd, &bmtmp->copld);)
        }
    }
    else
    {
        if (!(bm->copld.cnode.mln_Succ && bm->copld.cnode.mln_Succ->mln_Succ))
        {
            if (!IsListEmpty(&csd->c2ifragments))
            {
                bmtmp = BMDATFROMCOPSD(GetHead(&csd->c2ifragments));
                D(bug("[AmigaVideo] %s:     CopL @ 0x%p point to CopS @ 0x%p\n", __func__, &bm->copld, &bmtmp->copsd);)
                setcopperlisttail(csd, bm->copld.copper2_tail, bmtmp->bmcl->CopSStart, FALSE);                              // THIS CopL -> FIRST CopS
            }
            else
            {
                bmtmp = BMDATFROMCOPLD(GetHead(&csd->c2fragments));
                D(bug("[AmigaVideo] %s:     CopL @ 0x%p point to CopL @ 0x%p\n", __func__, &bm->copld, &bmtmp->copld);)
                setcopperlisttail(csd, bm->copld.copper2_tail, bmtmp->bmcl->CopLStart, FALSE);                              // THIS CopL -> FIRST CopL
            }
        }
    }

    if (!bmpred)
    {
        GfxBase->LOFlist = bm->bmcl->CopLStart;
        if ((bm->interlace) || (csd->interlaced))
            GfxBase->SHFlist = bm->bmcl->CopSStart;
        else
            GfxBase->SHFlist = bm->bmcl->CopLStart;
    }
    return TRUE;
}

/* remove a visible screen from the copperlist chains */
static VOID gfx_vblank_detachbm(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;
    struct copper2data *c2dp = NULL, *c2ds = NULL, *c2dip = NULL, *c2dis = NULL;
    struct amigabm_data *bmtmp;

    D(bug("[AmigaVideo] %s(0x%p)\n", __func__, bm));

    if (bm->copld.cnode.mln_Pred && bm->copld.cnode.mln_Pred->mln_Pred)
        c2dp = (struct copper2data *)bm->copld.cnode.mln_Pred;
    if (bm->copld.cnode.mln_Succ && bm->copld.cnode.mln_Succ->mln_Succ)
        c2ds = (struct copper2data *)bm->copld.cnode.mln_Succ;
    Remove(&bm->copld.cnode);
    D(bug("[AmigaVideo] %s:   removed bm->copld node\n", __func__);)

    if (csd->interlaced || bm->interlace)
    {
        if (bm->copsd.cnode.mln_Pred && bm->copsd.cnode.mln_Pred->mln_Pred)
            c2dip = (struct copper2data *)bm->copsd.cnode.mln_Pred;
        if (bm->copsd.cnode.mln_Succ && bm->copsd.cnode.mln_Succ->mln_Succ)
            c2dis = (struct copper2data *)bm->copsd.cnode.mln_Succ;
        Remove(&bm->copsd.cnode);
        D(bug("[AmigaVideo] %s:   removed bm->copsd node\n", __func__);)
    }

    if (c2dp && c2dp->cnode.mln_Pred)
    {
        if (c2ds)
        {
            bmtmp = BMDATFROMCOPLD(c2ds);
            setcopperlisttail(csd, c2dp->copper2_tail, bmtmp->bmcl->CopLStart, TRUE);
            D(bug("[AmigaVideo] %s: CopL @ 0x%p jmp to CopL @ 0x%p\n", __func__, c2dp, c2ds);)
        }
        else
        {
            D(bug("[AmigaVideo] %s: No screens follow us\n", __func__);)
            bmtmp = BMDATFROMCOPLD(GetHead(&csd->c2fragments));
            setcopperlisttail(csd, c2dp->copper2_tail, bmtmp->bmcl->CopLStart, FALSE);
            D(bug("[AmigaVideo] %s: CopL @ 0x%p point to CopL @ 0x%p\n", __func__, c2dp, &bmtmp->copld);)
        }

        if (csd->interlaced || bm->interlace)
        {
            if (c2dis)
            {
                bmtmp = BMDATFROMCOPSD(c2dis);
                setcopperlisttail(csd, c2dip->copper2_tail, bmtmp->bmcl->CopSStart, TRUE);
                D(bug("[AmigaVideo] %s: CopS @ 0x%p jmp to CopS @ 0x%p\n", __func__, c2dip, c2dis);)
            }
            else
            {
                bmtmp = BMDATFROMCOPLD(GetHead(&csd->c2fragments));
                setcopperlisttail(csd, c2dip->copper2_tail, bmtmp->bmcl->CopLStart, FALSE);
                D(bug("[AmigaVideo] %s: CopS @ 0x%p point to CopL @ 0x%p\n", __func__, c2dip, &bmtmp->copld);)
            }
        }
    }
    else
    {
        struct copper2data *c2t;
        UWORD *copstart;

        D(bug("[AmigaVideo] %s: screen-bitmap was first on copperlist chain\n", __func__);)
        if (c2ds)
        {
            bmtmp = BMDATFROMCOPLD(c2ds);

            /* first, replace any tail refrence ... */
            if (!IsListEmpty(&csd->c2ifragments))
            {
                c2t = (struct copper2data *)GetTail(&csd->c2ifragments);
                setcopperlisttail(csd, c2t->copper2_tail, bmtmp->bmcl->CopLStart, FALSE);
                D(bug("[AmigaVideo] %s: CopS @ 0x%p point to CopL @ 0x%p\n", __func__, c2t, &bmtmp->copsd);)
            }

            c2t = (struct copper2data *)GetTail(&csd->c2fragments);
            if (!IsListEmpty(&csd->c2ifragments))
            {
                setcopperlisttail(csd, c2t->copper2_tail, bmtmp->bmcl->CopSStart, FALSE);
                D(bug("[AmigaVideo] %s: CopL @ 0x%p point to CopS @ 0x%p\n", __func__, c2t, &bmtmp->copsd);)
            }
            else
            {
                setcopperlisttail(csd, c2t->copper2_tail, bmtmp->bmcl->CopLStart, FALSE);
                D(bug("[AmigaVideo] %s: CopL @ 0x%p point to CopL @ 0x%p\n", __func__, c2t, &bmtmp->copld);)
            }

            /* update the copper1 data to reflect the topmost screen */
            updatecopper1top(csd, bmtmp);

            /* now adjust the list(s) ... */
            GfxBase->LOFlist = bmtmp->bmcl->CopLStart;
            if (!IsListEmpty(&csd->c2ifragments))
            {
                bmtmp = BMDATFROMCOPSD(GetHead(&csd->c2ifragments));
                GfxBase->SHFlist = bmtmp->bmcl->CopSStart;
            }
            else
                GfxBase->SHFlist = bmtmp->bmcl->CopLStart;
        }
        else
        {
            D(bug("[AmigaVideo] %s: Nothing left to display!\n", __func__);)

            /* for our safety load the backup copperlist... */
            GfxBase->LOFlist = csd->copper2_backup;
            GfxBase->SHFlist = GfxBase->LOFlist;
        }
    }
}

static BOOL gfx_vblank_doupdatescroll(struct amigavideo_staticdata *csd)
{
    struct amigabm_data *bm, *bmsafe, *bmtmp;
    BOOL retval = FALSE;

    ForeachNodeSafe(csd->compositedbms, bm, bmsafe)
    {
        UWORD bmend;
        if (bm->node.ln_Succ && bm->node.ln_Succ->ln_Succ && (bm->node.ln_Succ == (struct Node *)csd->updatescroll))
        {
            UWORD olddisplayheight = bm->displayheight;

            /* adjust the screen situated above the moved screen-bitmap */
            bmend = csd->updatescroll->topedge - (csd->updatescroll->copld.extralines + 1);
            if ((bm->interlace) && (!csd->updatescroll->interlace))
                bmend <<= 1;
            else if ((!bm->interlace) && (csd->updatescroll->interlace))
                bmend >>= 1;

            setdisplayheight(csd, bm, (bmend - bm->topedge));

            /* only adjust if enough is visible - otherwise it will be obscured, and hidden in the next case... */
            if ((bm->displayheight != olddisplayheight) && (bm->displayheight > 1))
            {
                setcopperscrollbm(csd, bm);
                if (bm->bmucl)
                {

                    AmigaVideo_ParseCopperlist(csd, bm, bm->bmucl);
#if !USE_UCOP_DIRECT
                    D(bug("[AmigaVideo] %s:  -- copying user-copperlist data ...\n", __func__);)
                    CopyMemQuick(bm->bmucl->CopLStart, bm->copld.copper2_tail, bm->bmuclsize);
                    bm->copld.copper2_tail = (APTR)((IPTR)bm->copld.copper2_tail + bm->bmuclsize);
                    if ((bm->interlace) || (csd->interlaced))
                    {
                        if (bm->bmucl->CopSStart)
                            CopyMemQuick(bm->bmucl->CopSStart, bm->copsd.copper2_tail, bm->bmuclsize);
                        else
                            CopyMemQuick(bm->bmucl->CopLStart, bm->copsd.copper2_tail, bm->bmuclsize);
                        bm->copsd.copper2_tail = (APTR)((IPTR)bm->copsd.copper2_tail + bm->bmuclsize);
                    }
                    bm->bmucl->Flags |= (1<<15);
#endif
                }
            }
        }
        else if (bm == csd->updatescroll)
        {
            UWORD toptmp;
            retval = TRUE;

            if (bm->updtop < bm->topedge)
            {
                struct Node *tmpnode, *lastpred = &bm->node;
                UWORD adjust, topvis = bm->topedge - (bm->copld.extralines + 2);

                /* check if any of the obscured screens have become visible .. */
                ForeachNodeSafe(csd->obscuredbms, bmtmp, tmpnode)
                {
                    if (bmtmp->node.ln_Pri > bm->node.ln_Pri)
                    {
                        toptmp = bmtmp->topedge;
                        if ((bm->interlace) && (!bmtmp->interlace))
                        {
                            toptmp <<= 1;
                            adjust = (bmtmp->copld.extralines + 2) << 1;
                        }
                        else if ((!bm->interlace) && (bmtmp->interlace))
                        {
                            toptmp >>= 1;
                            adjust = (bmtmp->copld.extralines + 2) >> 1;
                        }

                        if (toptmp < topvis)
                        {
                            struct amigabm_data *bmpred = NULL;

                            if (lastpred->ln_Pred && lastpred->ln_Pred->ln_Pred)
                                bmpred = (struct amigabm_data *)lastpred->ln_Pred;

                            D(bug("[AmigaVideo] %s: attempt to make screen-bitmap with bmdata @ 0x%p visible\n", __func__, bmtmp, bmtmp->node.ln_Pri);)

                            if (gfx_vblank_attachbm(csd, bmtmp, bmpred))
                            {
                                D(bug("[AmigaVideo] %s: screen-bitmap became visible\n", __func__);)

                                topvis = toptmp - adjust;
                                Remove(&bmtmp->node);
                                Insert(csd->compositedbms, &bmtmp->node, lastpred);
                                lastpred = &bmtmp->node;
                            }
                        }
                    }
                }
            }
            else
            {
                /* check if we have obscured any screens .. */
                struct amigabm_data *bmtmp2;
                bmtmp = bm;
                while ((bmtmp = (struct amigabm_data *)GetPred(&bmtmp->node)))
                {
                    toptmp = bmtmp->topedge;
                    if ((bm->interlace) && (!bmtmp->interlace))
                        toptmp <<= 1;
                    else if ((!bm->interlace) && (bmtmp->interlace))
                        toptmp >>= 1;

                    if (toptmp >= bm->topedge - (bm->copld.extralines + 2))
                    {
                        bmtmp2 = (struct amigabm_data *)bmtmp->node.ln_Succ;

                        gfx_vblank_detachbm(csd, bmtmp);

                        D(bug("[AmigaVideo] %s: screen bitmap with bmdata @ 0x%p and depth %d is now obscured\n", __func__, bmtmp, bmtmp->node.ln_Pri);)

                        /* Remove the bitmap node and add it to the obscured list ... */
                        Remove(&bmtmp->node);
                        Enqueue(csd->obscuredbms, &bmtmp->node);
                        D(bug("[AmigaVideo] %s: * obscured bitmap has been moved from display lists\n", __func__);)

                        bmtmp = bmtmp2;
                    }
                }
            }

            /* adjust the moved screen-bitmap */
            if (bm->node.ln_Succ && bm->node.ln_Succ->ln_Succ)
            {
                bmtmp = (struct amigabm_data *)bm->node.ln_Succ;
                bmend = bmtmp->topedge - (bmtmp->copld.extralines + 1);
                if ((bm->interlace) && (!bmtmp->interlace))
                    bmend <<= 1;
                else if ((!bm->interlace) && (bmtmp->interlace))
                    bmend >>= 1;

                setdisplayheight(csd, bm, (bmend - bm->topedge));
            }
            else
                setdisplayheight(csd, bm, bm->viewportheight);
            if (bm->displayheight > 1)
            {
                setcopperscrollbm(csd, csd->updatescroll);
                if (bm->bmucl)
                {
                    AmigaVideo_ParseCopperlist(csd, bm, bm->bmucl);
#if !USE_UCOP_DIRECT
                    D(bug("[AmigaVideo] %s:  -- copying user-copperlist data ...\n", __func__);)
                    CopyMemQuick(bm->bmucl->CopLStart, bm->copld.copper2_tail, bm->bmuclsize);
                    bm->copld.copper2_tail = (APTR)((IPTR)bm->copld.copper2_tail + bm->bmuclsize);
                    if ((bm->interlace) || (csd->interlaced))
                    {
                        if (bm->bmucl->CopSStart)
                            CopyMemQuick(bm->bmucl->CopSStart, bm->copsd.copper2_tail, bm->bmuclsize);
                        else
                            CopyMemQuick(bm->bmucl->CopLStart, bm->copsd.copper2_tail, bm->bmuclsize);
                        bm->copsd.copper2_tail = (APTR)((IPTR)bm->copsd.copper2_tail + bm->bmuclsize);
                    }
                    bm->bmucl->Flags |= (1<<15);
#endif
                }
            }
            else
            {
                /* This screen has become obscured ... */
                gfx_vblank_detachbm(csd, bm);

                D(bug("[AmigaVideo] %s: active screen bitmap with bmdata @ 0x%p and depth %d is now obscured\n", __func__, bm, bm->node.ln_Pri);)

                /* Remove the bitmap node and add it to the obscured list ... */
                Remove(&bm->node);
                Enqueue(csd->obscuredbms, &bm->node);
                D(bug("[AmigaVideo] %s: * obscured bitmap for active screen has been moved from display lists\n", __func__);)
            }
            bm->updtop = bm->topedge;
            bm->updleft = bm->leftedge;
        }
    }
    D(bug("[AmigaVideo] %s: updatescroll processed\n", __func__);)
    return retval;
}
static AROS_INTH1(gfx_vblank, struct amigavideo_staticdata*, csd)
{
    AROS_INTFUNC_INIT

    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    BOOL lof = (custom->vposr & 0x8000) != 0;
    struct amigabm_data *bm;

    /* A palette-only update has no ChangeVPBitMap() call to publish it.
     * Commit it here, before this field's copper list is selected. */
    if (csd->compositedbms)
    {
        ForeachNode(csd->compositedbms, bm)
        {
            if (bm->palette_changed || bm->bitmap_changed)
                commitcopperchanges(csd, bm, FALSE);
        }
    }

    /* handle updatescroll first, since it may change the LOFlist/SHFlist pointers */
    if (csd->updatescroll) {
        D(bug("[AmigaVideo] %s: handling updatescroll\n", __func__);)

        if ((IsListEmpty(csd->compositedbms)) || (!gfx_vblank_doupdatescroll(csd)))
        {
            /* obscurred dragging bitmap has become visible */
            D(bug("[AmigaVideo] %s: re-displaying 0x%p\n", __func__, csd->updatescroll);)
        }
        csd->updatescroll = NULL;
    }

    /* is any displayed screen interlaced? */
    if (GfxBase->LOFlist != GfxBase->SHFlist) {
        custom->cop2lc = (ULONG)(lof ? GfxBase->LOFlist : GfxBase->SHFlist);
    } else {
        custom->cop2lc = (ULONG)GfxBase->LOFlist;
        /* We may be in SHF mode after switching interlace off. Fix it here. */
        if (!lof)
            custom->vposw = custom->vposr | 0x8000;
    }

    csd->framecounter++;
    if (csd->sprite) {
        UWORD *p = csd->sprite;
        p[0] = csd->spritepos;
        p[1 << csd->fmode_spr] = csd->spritectl;
    }
    
    for(int i=1; i<8; i++)
    {
      if (csd->new_sprite[i]) {
          UWORD *p = csd->new_sprite[i];
          p[0] = csd->new_spritepos[i];
          p[1 << csd->fmode_spr] = csd->new_spritectl[i];
      }
    }

    /* Classic applications retain and update their DMA streams directly,
     * including chained sprite images. Publish the caller's stream rather
     * than copying its pixels into the driver's cursor storage. */
    for (int i = 0; i < 8; i++)
    {
        UBYTE mask = 1 << i;
        struct SimpleSprite *sprite = GfxBase->SimpleSprites ?
                                      GfxBase->SimpleSprites[i] : NULL;
        if (sprite && (GfxBase->SpriteReserved & mask) &&
            !(GfxBase->ExtSprites & mask))
        {
            ULONG ptr = (ULONG)(sprite->posctldata ? sprite->posctldata : csd->sprite_null);
            UWORD *cop = csd->new_copper1_spritept[i];

            cop[0] = ptr >> 16;
            cop[2] = ptr;
            csd->classic_sprite_mask |= mask;
        }
        else if (csd->classic_sprite_mask & mask)
        {
            csd->classic_sprite_mask &= ~mask;
            if (i == 0)
                setspritevisible(csd, csd->cursorvisible);
            else
                new_setspritevisible(csd, csd->spritevisible[i], i);
        }
    }

    if (bqvar & BQ_BEAMSYNC) {
        bqvar |= BQ_MISSED;
        /* A queued beam target may fall beyond this field. Once the beam
         * wraps, restart the waiting queue rather than relying on an alarm
         * for a position that the field could not reach.
         */
        if (bqvar & BQ_BEAMSYNCWAITING)
            custom->intreq = INTF_SETCLR | INTF_BLIT;
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

/* The fixed head of copper list 1. An instruction flagged AGA or ECS is a
   NOP unless that chipset is there; the BPLCON flags OR the driver's value
   into the data word. */
#define C1_AGA      0x01
#define C1_ECS      0x02
#define C1_BPLCON0  0x04
#define C1_BPLCON2  0x08
#define C1_BPLCON3  0x10
struct copper1ins
{
    UWORD reg, val;
    UBYTE flags;
};
static const struct copper1ins copper1init[] = {
    { 0x00e0, 0, 0 },                                   // vsync_hblank
    { 0x0100, 0, C1_BPLCON0 },                          // diagstrt
    { 0x0106, (1 << 6), C1_BPLCON3 },                   // default display bplcon3 (ecs sprites)
    { 0x0180, DEFAULT_BORDER_GRAY, 0 },                 // Pen = 0
    { 0x0106, (1 << 9) | (1 << 6), C1_BPLCON3 | C1_AGA },   // high bits of the palette entry
    { 0x0180, DEFAULT_BORDER_GRAY, C1_AGA },            // Pen = 0
    { 0x0106, (1 << 6), C1_BPLCON3 | C1_AGA },          // default display bplcon3 again
    { 0x01fc, 0, C1_ECS },                              // fmode 0
    { 0x008e, (1 << 8) | 0x81, C1_ECS },                // (DIWSTRT) Display window start
    { 0x0090, (2 << 8) | 0x81, C1_ECS },                // (DIWSTOP) Display window stop
    { 0x01e4, 0, C1_ECS },                              // (DIWHIGH) Display window
    { 0x0092, 0, C1_ECS },                              // (DDFSTRT)
    { 0x0094, 0, C1_ECS },                              // (DDFSTOP)
    { 0x0104, 0, C1_BPLCON2 | C1_ECS },                 // the screens bplcon2
};

VOID initcustom(struct amigavideo_staticdata *csd)
{
    UBYTE i;
    UWORD *c;
    UWORD vposr, val;
    struct GfxBase *GfxBase;
    struct Library *OOPBase;
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    volatile struct CIA *ciab = (struct CIA*)0xbfd000;

    D(bug("[AmigaVideo] %s()\n", __func__));

    /* csd->cs_OOPBase was already set up.
     * See amigavideo.conf's 'oopbase_field' config
     */
    OOPBase = csd->cs_OOPBase;
    csd->cs_HiddBitMapBase = OOP_GetMethodID(IID_Hidd_BitMap, 0);
    csd->cs_HiddGfxBase = OOP_GetMethodID(IID_Hidd_Gfx, 0);
    csd->cs_HiddDMEnumBase = OOP_GetMethodID(IID_Hidd_DMEnum, 0);

    csd->cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!csd->cs_UtilityBase)
        Alert(AT_DeadEnd | AN_Hidd | AG_OpenLib | AO_UtilityLib);
    csd->cs_GfxBase = TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    if (!csd->cs_GfxBase)
        Alert(AT_DeadEnd | AN_Hidd | AG_OpenLib | AO_GraphicsLib);
    GfxBase = ((struct GfxBase *)csd->cs_GfxBase);
    GfxBase->cia = OpenResource("ciab.resource");

    vposr = custom->vposr;
    csd->aga = (vposr & 0x0200) == 0x0200;
    csd->ecs_agnus = (vposr & 0x2000) == 0x2000;
    val = custom->deniseid;
    custom->deniseid = custom->dmaconr;
    if (val == custom->deniseid) {
        custom->deniseid = custom->dmaconr ^ 0x8000;
        if (val == custom->deniseid) {
            if ((val & (2 + 8)) == 8)
                csd->ecs_denise = TRUE;
        }
    }

    /* Reset now we have the bases */
    resetcustom(csd);
    resetsprite(csd);
    
    for (i = 1; i < 8; i++)
        new_resetsprite(csd, i);

    csd->inter.is_Code         = (APTR)gfx_vblank;
    csd->inter.is_Data         = csd;
    csd->inter.is_Node.ln_Name = "GFX VBlank server";
    csd->inter.is_Node.ln_Pri  = 25;
    csd->inter.is_Node.ln_Type = NT_INTERRUPT;
    AddIntServer(INTB_VERTB, &csd->inter);

    /* There are programs that take over the system and
     * assume SysBase->IntVects[BLITTER].iv_Data = GfxBase!
     */
    GfxBase->bltsrv.is_Code         = (APTR)gfx_blit;
    GfxBase->bltsrv.is_Data         = GfxBase;
    GfxBase->bltsrv.is_Node.ln_Name = "Blitter";
    GfxBase->bltsrv.is_Node.ln_Type = NT_INTERRUPT;
    SetIntVector(INTB_BLIT, &GfxBase->bltsrv);
    custom->intena = INTF_BLIT;

    // CIA-B TOD counts scanlines */
    GfxBase->timsrv.is_Code = (APTR)gfx_beamsync;
    GfxBase->timsrv.is_Data = csd;
    GfxBase->timsrv.is_Node.ln_Name = "Beamsync";
    GfxBase->timsrv.is_Node.ln_Type = NT_INTERRUPT;
    Disable();
    AddICRVector(GfxBase->cia, 2, &GfxBase->timsrv);
    AbleICR(GfxBase->cia, 1 << 2);
    ciab->ciacrb |= 0x80;
    ciab->ciatodhi = 0;
    /* TOD/ALARM CIA bug: http://eab.abime.net/showpost.php?p=277315&postcount=10 */
    ciab->ciatodmid = BEAMSYNC_ALARM >> 8;
    ciab->ciatodlow = BEAMSYNC_ALARM & 0xff;
    ciab->ciacrb &= ~0x80;
    ciab->ciatodhi = 0;
    ciab->ciatodmid = 0;
    ciab->ciatodlow = 0;
    AbleICR(GfxBase->cia, 0x80 | (1 << 2));
    Enable();

    GfxBase->NormalDisplayColumns = 640;
    GfxBase->NormalDisplayRows = (GfxBase->DisplayFlags & NTSC) ? REZ_Y_MIN : (REZ_Y_MIN + REZ_PAL_LINES);
    GfxBase->MaxDisplayColumn = 640;
    GfxBase->MaxDisplayRow = (GfxBase->DisplayFlags & NTSC) ? REZ_Y_MIN : (REZ_Y_MIN + REZ_PAL_LINES);

    csd->startx = STANDARD_VIEW_X;
    csd->starty = STANDARD_VIEW_Y;

    csd->max_colors = csd->aga ? 256 : 32;

    csd->sprite_null = AllocMem((2 << 3) + (46 * (sizeof(WORD) << 1)), MEMF_CLEAR | MEMF_CHIP);
    csd->copper1 = (APTR)((IPTR)csd->sprite_null + (2 << 3));
    c = csd->copper1;

    for (i = 0; i < sizeof(copper1init) / sizeof(copper1init[0]); i++) {
        const struct copper1ins *ci = &copper1init[i];
        UWORD reg = ci->reg, v = ci->val;

        if ((ci->flags & C1_AGA) && !csd->aga_enabled)
            reg = 0x01fe;
        if ((ci->flags & C1_ECS) && !csd->ecs_denise)
            reg = 0x01fe;
        if (ci->flags & C1_BPLCON0)
            v |= csd->bplcon0_null;
        if (ci->flags & C1_BPLCON2)
            v |= csd->bplcon2;
        if (ci->flags & C1_BPLCON3)
            v |= csd->bplcon3;
        COPPEROUT(c, reg, v)
    }

    /* initialize SPRxPOS (sprfix) */
    for (i = 0; i < 8; i++) {
        COPPEROUT(c, 0x0140 + (i << 3), 0)
    }
    
    /* set the initial sprite to a blank cursor (sprstrtup) */
    for (i = 0; i < 8; i++) {
        COPPEROUT(c, 0x0120 + (i << 2), (UWORD)(((ULONG)csd->sprite_null) >> 16))
        if (i == 0)
            csd->copper1_spritept = &c[-1];
        csd->new_copper1_spritept[i] = &c[-1];
        COPPEROUT(c, 0x0122 + (i << 2), (UWORD)(((ULONG)csd->sprite_null) >> 0))
    }

    // wait14
    COPPEROUT(c, 0x0c03, 0xfffe)        // Wait for VPOS = 0c, HPOS = 2

    // norm_hblank
    COPPEROUT(c, 0x0c03, 0xfffe)        // Wait for VPOS = 0c, HPOS = 2

    // jump
    COPPEROUT(c, 0x008a, 0x0000)        // COPJMP2

    csd->copper2_backup = c;

    // wait_forever
    COPPEROUT(c, 0x0106, csd->bplcon3)  // Push the display bplcon3 again
    COPPEROUT(c, 0xffff, 0xfffe)

    GfxBase->LOFlist = GfxBase->SHFlist = csd->copper2_backup;

    custom->cop1lc = (ULONG)csd->copper1;
    custom->cop2lc = (ULONG)csd->copper2_backup;

    /*
     * Keep raster DMA enabled even while the null view has no bitplanes,
     * matching Kickstart.  Hardware-takeover programs may save DMACONR and
     * restore it after installing their own bitplanes.
     */
    custom->dmacon = 0x8000 | 0x0100 | 0x0080 | 0x0040 | 0x0020;
    
    GfxBase->copinit = (struct copinit*)csd->copper1;

    D(bug("[AmigaVideo] %s: Copperlist0 %p (%d bytes)\n", __func__, csd->copper1, 46 * (sizeof(WORD) << 1)));
}

// This function unconditionally enables AGA features. Only call if you are SetChipRev!
VOID AmigaVideoCl__Hidd_AmigaGfx__EnableAGA(OOP_Class *cl, OOP_Object *o, void *msg)
{
    struct amigabm_data *bm, *bmnext;
    struct amigavideo_staticdata *csd = CSD(cl);
    IPTR copperlist_end;
    csd->aga_enabled = TRUE;
    // Recalculate copper lists for all visible screens now that AGA is on.
    // This should be enough to cover the sane cases when SetChipRev is called.
    // Interlace is ignored. Don't go open interlace screens before activating AGA!
    (void)bm; (void)bmnext; (void)copperlist_end;
    /*
     * Existing screens keep their pre-AGA copper lists: those chip-RAM
     * allocations were sized without the AGA colour/fmode entries, so
     * rebuilding them in place with AGA-sized content overruns the
     * allocation and tramples low chip memory (including AbsExecBase).
     * They continue to display exactly as they did before this call.
     * Screens opened from now on are built with AGA-sized lists.
     */
}

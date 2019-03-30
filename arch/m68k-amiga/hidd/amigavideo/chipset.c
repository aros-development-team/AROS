/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cia.h>

#include <exec/libraries.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hardware/cia.h>
#include <hidd/gfx.h>
#include <graphics/modeid.h>

#include "amigavideo_hidd.h"
#include "amigavideo_bitmap.h"
#include "chipset.h"

#include <aros/debug.h>

#define BPLCONMASK 0x8a55

static const UBYTE fetchunits[] = { 3,3,3,0, 4,3,3,0, 5,4,3,0 };
static const UBYTE fm_maxplanes[] = { 3,2,1,0, 3,3,2,0, 3,3,3,0 };

/* reset to OCS defaults */
void resetcustom(struct amigavideo_staticdata *csd)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;

    D(
      bug("[AmigaVideo] %s()\n", __func__);
      bug("[AmigaVideo] %s: GfxBase @ 0x%p\n", __func__, GfxBase);
     )

    GfxBase->system_bplcon0 &= ~BPLCONMASK;
    GfxBase->system_bplcon0 |= 0x0200;
    D(bug("[AmigaVideo] %s: bplcon0 = %04x\n", __func__, GfxBase->system_bplcon0));

    custom->fmode = 0x0000;
    custom->bplcon0 = GfxBase->system_bplcon0;
    custom->bplcon1 = 0x0000;
    custom->bplcon2 = 0x0024;
    custom->bplcon3 = 0x0c00;
    custom->bplcon4 = 0x0011;
    custom->vposw = 0x8000;
    custom->color[0] = 0x0444;

    // Use AGA modes and create AGA copperlists only if AGA is "enabled"
    csd->aga_enabled = csd->aga && GfxBase->ChipRevBits0 == SETCHIPREV_AA;
}

static void waitvblank(struct amigavideo_staticdata *csd)
{
    // ugly busy loop for now..
    UWORD fc = csd->framecounter;
    while (fc == csd->framecounter);
}
 
static void setnullsprite(struct amigavideo_staticdata *csd)
{
    if (csd->copper1_spritept) {
        UWORD *p = csd->sprite_null;
        csd->copper1_spritept[0] = (UWORD)(((ULONG)p) >> 16);
        csd->copper1_spritept[2] = (UWORD)(((ULONG)p) >> 0);
    }
 }
 
void resetsprite(struct amigavideo_staticdata *csd)
{
    UWORD *sprite = csd->sprite;
    setnullsprite(csd);
    csd->sprite = NULL;
    FreeMem(sprite, csd->spritedatasize);
    csd->sprite_width = csd->sprite_height = 0;
}

static void setfmode(struct amigavideo_staticdata *csd)
{
    UWORD fmode;
    fmode  =  csd->fmode_bpl == 2 ? 3 : csd->fmode_bpl;
    fmode |= (csd->fmode_spr == 2 ? 3 : csd->fmode_spr) << 2;
    if (csd->copper2.copper2_fmode) {
        *csd->copper2.copper2_fmode = fmode;
        if (csd->interlace)
            *csd->copper2i.copper2_fmode = fmode;
    }
}

static void setcoppercolors(struct amigavideo_staticdata *csd)
{
    UWORD i;
 
    if (!csd->copper2.copper2_palette)
        return;
    if (csd->aga && csd->aga_enabled) {
        UWORD off = 1;
        for (i = 0; i < csd->use_colors; i++) {
            UWORD vallo, valhi;
            UBYTE r = csd->palette[i * 3 + 0];
            UBYTE g = csd->palette[i * 3 + 1];
            UBYTE b = csd->palette[i * 3 + 2];
            if ((i & 31) == 0)
                off += 2;
            valhi = ((r & 0xf0) << 4) | ((g & 0xf0)) | ((b & 0xf0) >> 4);
            vallo = ((r & 0x0f) << 8) | ((g & 0x0f) << 4) | ((b & 0x0f));
            csd->copper2.copper2_palette[i * 2 + off] = valhi;
            csd->copper2.copper2_palette_aga_lo[i * 2 + off] = vallo;
            if (csd->interlace) {
                csd->copper2i.copper2_palette[i * 2 + off] = valhi;
                csd->copper2i.copper2_palette_aga_lo[i * 2 + off] = vallo;
            }	
        }   
    } else if (csd->res == 2 && !csd->aga) {
        /* ECS "scrambled" superhires */
        for (i = 0; i < csd->use_colors; i++) {
            UBYTE offset = i < 16 ? 0 : 16;
            UBYTE c1 = (i & 3) + offset;
            UBYTE c2 = ((i >> 2) & 3) + offset;
            UWORD val1 = ((csd->palette[c1 * 3 + 0] >> 4) << 8) | ((csd->palette[c1 * 3 + 1] >> 4) << 4) | ((csd->palette[c1 * 3 + 2] >> 4) << 0);
            UWORD val2 = ((csd->palette[c2 * 3 + 0] >> 4) << 8) | ((csd->palette[c2 * 3 + 1] >> 4) << 4) | ((csd->palette[c2 * 3 + 2] >> 4) << 0);
            UWORD val = (val1 & 0xccc) | ((val2 & 0xccc) >> 2);
            csd->copper2.copper2_palette[i * 2 + 1] = val;
            if (csd->interlace)
                csd->copper2i.copper2_palette[i * 2 + 1] = val;
        }
        
    } else {
        for (i = 0; i < csd->use_colors; i++) {
            UWORD val = ((csd->palette[i * 3 + 0] >> 4) << 8) | ((csd->palette[i * 3 + 1] >> 4) << 4) | ((csd->palette[i * 3 + 2] >> 4) << 0);
            csd->copper2.copper2_palette[i * 2 + 1] = val;
            if (csd->interlace)
                csd->copper2i.copper2_palette[i * 2 + 1] = val;
        }
    }
}

static void setpalntsc(struct amigavideo_staticdata *csd, ULONG modeid)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;

    csd->palmode = (GfxBase->DisplayFlags & NTSC) == 0;
    if (!csd->ecs_agnus)	
        return;
    if ((modeid & MONITOR_ID_MASK) == PAL_MONITOR_ID) {
        custom->beamcon0 = 0x0020;
        csd->palmode = TRUE;
    } else if ((modeid & MONITOR_ID_MASK) == NTSC_MONITOR_ID) {
        custom->beamcon0 = 0x0000;
        csd->palmode = FALSE;
    } else {
        custom->beamcon0 = (GfxBase->DisplayFlags & NTSC) ? 0x0000 : 0x0020;
    }
}

void resetmode(struct amigavideo_staticdata *csd)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;

    D(bug("[AmigaVideo] %s()\n", __func__));

    csd->disp = NULL;

    custom->dmacon = 0x0100;
    setpalntsc(csd, 0);

    custom->cop2lc = (ULONG)csd->copper2_backup;
    custom->copjmp2 = 0;

    waitvblank(csd);

    FreeVec(csd->copper2.copper2);
    csd->copper2.copper2 = NULL;
    FreeVec(csd->copper2i.copper2);
    csd->copper2i.copper2 = NULL;

    GfxBase->LOFlist = GfxBase->SHFlist = csd->copper2_backup;

    resetcustom(csd);

    csd->depth = 0;
}

/* Use nominal screen height. Overscan is not supported yet. */
static WORD limitheight(struct amigavideo_staticdata *csd, WORD y, BOOL lace, BOOL maxlimit)
{
    if (lace)
        y /= 2;
    if (csd->palmode) {
        if (maxlimit && y > 311)
            y = 311;
        else if (!maxlimit && y > 256)
            y = 256;
    } else {
        if (maxlimit && y > 261)
            y = 261;
        else if (!maxlimit && y > 200)
            y = 200;
    }
    if (lace)
        y *= 2;
    return y;
}

static void setcopperscroll2(struct amigavideo_staticdata *csd, struct amigabm_data *bm, struct copper2data *c2d, BOOL odd)
{
    UWORD *copptr = c2d->copper2_scroll, *copbpl;
    WORD xscroll, yscroll;
    WORD x, y;
    WORD ystart, yend, i, xdelay;
    WORD xmaxscroll,  modulo, ddfstrt, fmodewidth, minearly;
    LONG offset;
    
    fmodewidth = 16 << csd->fmode_bpl;
    x = bm->leftedge;
    y = csd->starty + (bm->topedge >> csd->interlace);
    
    yscroll = 0;
    if (y < 10) {
        yscroll = y - 10;
        y = 10;
    }

    xmaxscroll = 1 << (1 + csd->fmode_bpl);
    xdelay = x & (fmodewidth - 1);
    xscroll = -x;
    
    yend = y + (bm->displayheight >> csd->interlace);
    yend = limitheight(csd, yend, FALSE, TRUE);
    ystart = y - csd->extralines;
        
    modulo = (csd->interlace ? bm->bytesperrow : 0) + csd->modulo;
    ddfstrt = csd->ddfstrt;

    offset = ((xscroll + (xmaxscroll << 3) - 1) >> 3) & ~(xmaxscroll - 1);
    offset -= (yscroll * bm->bytesperrow) << (csd->interlace ? 1 : 0);

    minearly = 1 << fetchunits[csd->fmode_bpl * 4 + csd->res];
    if (xdelay) {
        ddfstrt -= minearly;
        modulo -= (minearly << csd->res) / 4;
        offset -= (minearly << csd->res) / 4;
    }

    copptr[1] = (y << 8) | (csd->startx); //(y << 8) + (x + 1);
    copptr[3] = (yend << 8) | ((csd->startx + 0x140) & 0xff); //((y + (bm->rows >> csd->interlace)) << 8) + ((x + 1 + (bm->width >> csd->res)) & 0x00ff);
    copptr[5] = ((y >> 8) & 7) | (((yend >> 8) & 7) << 8) | 0x2000;

    copbpl = c2d->copper2_bpl;
    for (i = 0; i < bm->depth; i++) {
        ULONG pptr = (ULONG)(bm->pbm->Planes[csd->bploffsets[i]]);
        if (csd->interlace && odd)
            pptr += bm->bytesperrow;
        pptr += offset;
        copbpl[1] = (UWORD)(pptr >> 16);
        copbpl[3] = (UWORD)(pptr >> 0);
        copbpl += 4;
    }

    xdelay <<= 2 - csd->res;
    copptr[11] =
          (((xdelay >> 2) & 0x0f) << 0) | (((xdelay >> 2) & 0x0f) << 4)
        | ((xdelay >> 6) << 10) | ((xdelay >> 6) << 14)
        | ((xdelay & 3) << 8) | ((xdelay & 3) << 12);

    copptr[7] = ddfstrt;
    copptr[9] = csd->ddfstop;
    copptr[13] = modulo;
    copptr[15] = modulo;

    yend = y + bm->displayheight + yscroll;
    yend = limitheight(csd, yend, FALSE, TRUE);
    copptr = c2d->copper2_bplcon0;
    copptr[4] = (yend << 8) | 0x05;
    if (yend < 256 || ystart >= 256) {
        copptr[2] = 0x00df;
        copptr[3] = 0x00fe;
    } else {
        copptr[2] = 0xffdf;
        copptr[3] = 0xfffe;
    }

    copptr = c2d->copper2;
    if (ystart >= 256)
        copptr[0] = 0xffdf;
    else
        copptr[0] = 0x01fe;
    copptr[2] = (ystart << 8) | 0x05;
    copptr = c2d->copper2_bplcon0;
    copptr[-2] = (y << 8) | 0x05;
}

static void setcopperscroll(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    setcopperscroll2(csd, bm, &csd->copper2, FALSE);
    if (csd->interlace)
        setcopperscroll2(csd, bm, &csd->copper2i, TRUE);
}

static UWORD get_copper_list_length(struct amigavideo_staticdata *csd, UBYTE depth)
{
    UWORD v;

    if (csd->aga && csd->aga_enabled) {
        v = 1000 + ((1 << depth) + 1 + (1 << depth) / 32 + 1) * 2;
    } else {
        v = 1000;
    }
    return v * 2;
}

static void createcopperlist(struct amigavideo_staticdata *csd, struct amigabm_data *bm, struct copper2data *c2d, BOOL lace)
{
    struct GfxBase *GfxBase = (struct GfxBase *)csd->cs_GfxBase;
    volatile WORD *system_bplcon0 = (volatile WORD *)&GfxBase->system_bplcon0;
    UWORD *c;
    UWORD i;
    UWORD bplcon0, bplcon0_res;
    ULONG pptr;

    D(bug("[AmigaVideo] %s()\n", __func__));
    D(bug("[AmigaVideo] %s: GfxBase @ 0x%p\n", __func__, GfxBase));
    D(bug("[AmigaVideo] %s: system_bplcon0 @ 0x%p\n", __func__, system_bplcon0));

    c = c2d->copper2;
    D(bug("[AmigaVideo] %s: Copperlist%d %p\n", __func__,  lace ? 2 : 1, c));

    bplcon0_res = *system_bplcon0 & ~BPLCONMASK;
    D(bug("[AmigaVideo] %s: bplcon0_res = %04x\n", __func__, bplcon0_res));

    if (csd->res == 1)
         bplcon0_res |= 0x8000;
    else if (csd->res == 2)
        bplcon0_res |= 0x0040;
    else
        bplcon0_res = 0;

    csd->bplcon0_null = 0x0201 | (csd->interlace ? 4 : 0) | bplcon0_res;
    csd->bplcon3 = ((csd->sprite_res + 1) << 6) | 2; // spriteres + bordersprite

    *c++ = 0x01fe;
    *c++ = 0xfffe;
    *c++ = 0xffff;
    *c++ = 0xfffe;

    *c++ = 0x0100;
    *c++ = csd->bplcon0_null;

    c2d->copper2_bpl = c;
    for (i = 0; i < bm->depth; i++) {
        pptr = (ULONG)(bm->pbm->Planes[csd->bploffsets[i]]);
        if (lace)
            pptr += bm->bytesperrow;
        *c++ = 0xe0 + i * 4;
        *c++ = (UWORD)(pptr >> 16);
        *c++ = 0xe2 + i * 4;
        *c++ = (UWORD)(pptr >> 0);
    }

    csd->use_colors = 1 << bm->depth;
    // need to update sprite colors
    if (csd->use_colors < 16 + 4)
        csd->use_colors = 16 + 4;
    if (csd->res == 2 && !csd->aga)
        csd->use_colors = 32; /* ECS "scrambled" superhires */
    
    if (csd->use_colors > 32 && (csd->modeid & EXTRAHALFBRITE_KEY))
        csd->use_colors = 32;
    if (csd->modeid & HAM_KEY) {
        if (bm->depth <= 6)
            csd->use_colors = 16 + 4;
        else
            csd->use_colors = 64;
    }

    c2d->copper2_scroll = c;
    *c++ = 0x008e;
    *c++ = 0;
    *c++ = 0x0090;
    *c++ = 0;
    *c++ = 0x01e4;
    *c++ = 0;
    *c++ = 0x0092;
    *c++ = 0;
    *c++ = 0x0094;
    *c++ = 0;
    *c++ = 0x0102;
    *c++ = 0;
    *c++ = 0x0108;
    *c++ = 0;
    *c++ = 0x010a;
    *c++ = 0;
    *c++ = 0x0104;
    *c++ = 0x0024 | ((csd->aga && !(csd->modeid & EXTRAHALFBRITE_KEY)) ? 0x0200 : 0);

    c2d->copper2_fmode = NULL;
    if (csd->aga && csd->aga_enabled) {
        *c++ = 0x010c;
        *c++ = 0x0011;
        *c++ = 0x01fc;
        c2d->copper2_fmode = c;
        *c++ = 0;
    }

    bplcon0 = csd->bplcon0_null;
    if (bm->depth > 7)
        bplcon0 |= 0x0010;
    else
        bplcon0 |= bm->depth << 12;
    if (csd->modeid & HAM_KEY)
        bplcon0 |= 0x0800;

    *system_bplcon0 = (*system_bplcon0 & ~BPLCONMASK) | bplcon0;
    D(bug("[AmigaVideo] %s: system_bplcon0 = %04x\n", __func__, *system_bplcon0));

    c2d->copper2_palette = c;
    if (csd->aga && csd->aga_enabled) {
        // hi
        for (i = 0; i < csd->use_colors; i++) {
            UBYTE agac = i & 31;
            if (agac == 0) {
                *c++ = 0x106;
                *c++ = csd->bplcon3 | ((i / 32) << 13);
            }
            *c++ = 0x180 + agac * 2;
            *c++ = 0x000;
        }
        c2d->copper2_palette_aga_lo = c;
        // lo
        for (i = 0; i < csd->use_colors; i++) {
            UBYTE agac = i & 31;
            if (agac == 0) {
                *c++ = 0x106;
                *c++ = csd->bplcon3 | ((i / 32) << 13) | 0x0200;
            }
            *c++ = 0x180 + agac * 2;
            *c++ = 0x000;
        }
        *c++ = 0x106;
        *c++ = csd->bplcon3;
    } else {
        // ocs/ecs
        for (i = 0; i < csd->use_colors; i++) {
            *c++ = 0x180 + i * 2;
            *c++ = 0x000;
        }
    }

    csd->extralines = (c - c2d->copper2) / 112 + 1;

    *c++ = 0xffff;
    *c++ = 0xfffe;
    c2d->copper2_bplcon0 = c;
    *c++ = 0x0100;
    *c++ = bplcon0;

    *c++ = 0xffff;
    *c++ = 0xfffe;
    *c++ = 0xffff;
    *c++ = 0xfffe;
    
    *c++ = 0x0100;
    *c++ = csd->bplcon0_null;

   if (csd->interlace) {
        ULONG nextptr = (ULONG)(lace ? csd->copper2.copper2 : csd->copper2i.copper2);
        *c++ = 0x0084;
        *c++ = (UWORD)(nextptr >> 16);
        *c++ = 0x0086;
        *c++ = (UWORD)(nextptr >> 0);
    }
    *c++ = 0xffff;
    *c++ = 0xfffe;

}

BOOL setbitmap(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    csd->width = bm->width;
    csd->height = csd->interlace ? (bm->height + 1) / 2 : bm->height;
    csd->modulo = bm->bytesperrow - csd->modulopre / (4 >> csd->res);
    csd->modulo &= ~((2 << csd->fmode_bpl) - 1);
    csd->updatescroll = bm;
    csd->depth = bm->depth;
    setcopperscroll(csd, bm);

    D(bug("setbitmap bm=%x mode=%08x w=%d h=%d d=%d bpr=%d\n",
        bm, csd->modeid, bm->width, bm->height, bm->depth, bm->bytesperrow));
        return TRUE;
}

BOOL setmode(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;
    UWORD ddfstrt, ddfstop;
    UBYTE fetchunit, maxplanes;
    UWORD bplwidth, viewwidth;
    UBYTE i;

    if (csd->disp == bm)
        return TRUE;

    resetmode(csd);

    csd->res = 0;
    if ((csd->modeid & SUPER_KEY) == SUPER_KEY)
        csd->res = 2;
    else if ((csd->modeid & SUPER_KEY) == HIRES_KEY)
        csd->res = 1;
    csd->interlace = (csd->modeid & LORESLACE_KEY) ? 1 : 0;
    csd->fmode_bpl = csd->aga && csd->aga_enabled ? 2 : 0;

    fetchunit = fetchunits[csd->fmode_bpl * 4 + csd->res];
    maxplanes = fm_maxplanes[csd->fmode_bpl * 4 + csd->res];

    D(bug("res %d fmode %d depth %d maxplanes %d aga %d agae %d\n",
        csd->res, csd->fmode_bpl, bm->depth, maxplanes, csd->aga, csd->aga_enabled));

    if (bm->depth > (1 << maxplanes)) {
        if (csd->aga && !csd->aga_enabled) {
            // Enable AGA if requesting AGA only mode.
            // This is a compatibility hack because our display
            // database currently contains all AGA modes even if AGA
            // is "disabled".
            GfxBase->ChipRevBits0 = SETCHIPREV_AA;
            csd->aga_enabled = TRUE;
            csd->fmode_bpl = csd->aga && csd->aga_enabled ? 2 : 0;
            fetchunit = fetchunits[csd->fmode_bpl * 4 + csd->res];
            maxplanes = fm_maxplanes[csd->fmode_bpl * 4 + csd->res];
        }
        if (bm->depth > (1 << maxplanes))
            return FALSE;
    }

    viewwidth = bm->width;
    // use nominal width for now
    if ((viewwidth << csd->res) > 320)
        viewwidth = 320 << csd->res;

    D(bug("setmode bm=%x mode=%08x w=%d h=%d d=%d bpr=%d fu=%d\n",
        bm, csd->modeid, bm->width, bm->height, bm->depth, bm->bytesperrow, fetchunit));
    
    bplwidth = viewwidth >> (csd->res + 1);
    ddfstrt = (csd->startx / 2) & ~((1 << fetchunit) - 1);
    ddfstop = ddfstrt + ((bplwidth + ((1 << fetchunit) - 1) - 2 * (1 << fetchunit)) & ~((1 << fetchunit) - 1));
    csd->modulopre = ddfstop + 2 * (1 << fetchunit) - ddfstrt;
    ddfstrt -= 1 << maxplanes;
    csd->ddfstrt = ddfstrt;
    csd->ddfstop = ddfstop;

    for (i = 0; i < 8; i++)
        csd->bploffsets[i] = i;
    if ((csd->modeid & HAM_KEY) && bm->depth > 6) {
        csd->bploffsets[0] = 6;
        csd->bploffsets[1] = 7;
        for (i = 0; i < 6; i++)
            csd->bploffsets[i + 2] = i;
    }

    csd->copper2.copper2 = AllocVec(get_copper_list_length(csd, bm->depth), MEMF_CLEAR | MEMF_CHIP);
    if (csd->interlace)
        csd->copper2i.copper2 = AllocVec(get_copper_list_length(csd, bm->depth), MEMF_CLEAR | MEMF_CHIP);
    createcopperlist(csd, bm, &csd->copper2, FALSE);
    if (csd->interlace) {
        createcopperlist(csd, bm, &csd->copper2i, TRUE);
    }
 
    setfmode(csd);
    setpalntsc(csd, csd->modeid);
    custom->bplcon0 = csd->bplcon0_null;

    bm->displaywidth = viewwidth;
    bm->displayheight = limitheight(csd, bm->height, csd->interlace, FALSE);

    setbitmap(csd, bm);

        GfxBase->LOFlist = csd->copper2.copper2;
        GfxBase->SHFlist = csd->interlace ? csd->copper2i.copper2 : csd->copper2.copper2;
        custom->dmacon = 0x8100;

    setcoppercolors(csd);
    setspritepos(csd, csd->spritex, csd->spritey);

    csd->disp = bm;
    return 1;
 }

UBYTE av__PickPen(struct amigavideo_staticdata *csd, ULONG pixel)
{
    UBYTE retval = 1;
    return retval;
}

BOOL setsprite(OOP_Class *cl, OOP_Object *o, WORD width, WORD height, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *OOPBase = csd->cs_OOPBase;
    OOP_MethodID HiddGfxBase = csd->cs_HiddGfxBase;
    OOP_MethodID HiddBitMapBase = csd->cs_HiddBitMapBase;
    OOP_Object *bmPFObj = NULL;
    HIDDT_PixelFormat *bmPF;
    IPTR pf, bmcmod;
    UWORD fetchsize;
    UWORD bitmapwidth = width;
    UWORD y, *p;

    OOP_GetAttr(msg->shape, aHidd_BitMap_PixFmt, (IPTR*)&bmPFObj);
    OOP_GetAttr(bmPFObj, aHidd_PixFmt_ColorModel, &bmcmod);
    if (bmcmod == vHidd_ColorModel_TrueColor)
    {
        OOP_GetAttr(bmPFObj, aHidd_PixFmt_StdPixFmt, (IPTR*)&pf);
        bmPF = HIDD_Gfx_GetPixFmt(o, pf);
    }

    if (csd->aga && csd->aga_enabled && width > 16)
        csd->fmode_spr = 2;
    else
        csd->fmode_spr = 0;
    fetchsize = 2 << csd->fmode_spr;
    width = 16 << csd->fmode_spr;

    if (width != csd->sprite_width || height != csd->sprite_height) {
        resetsprite(csd);
        csd->spritedatasize = fetchsize * 2 + fetchsize * height * 2 + fetchsize * 2;
        csd->sprite = AllocMem(csd->spritedatasize, MEMF_CHIP | MEMF_CLEAR);
        if (!csd->sprite)
            return FALSE;
        csd->sprite_width = width;
        csd->sprite_height = height;
        csd->sprite_offset_x = msg->xoffset;
        csd->sprite_offset_y = msg->yoffset;
    }
    p = csd->sprite;
    p += fetchsize;
    for(y = 0; y < height; y++) {
        UWORD xx, xxx, x;
        for (xx = 0, xxx = 0; xx < width; xx += 16, xxx++) {
            UWORD pix1 = 0, pix2 = 0;
            for(x = 0; x < 16; x++) {
                UBYTE c = 0;
                if (xx + x < bitmapwidth)
                {
                    if (bmcmod != vHidd_ColorModel_TrueColor)
                        c = HIDD_BM_GetPixel(msg->shape, x, y);
                    else
                    {
                        HIDDT_Pixel pix = HIDD_BM_GetPixel(msg->shape, x, y);
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
    setspritepos(csd, csd->spritex, csd->spritey);
    setspritevisible(csd, csd->cursorvisible);
    return TRUE;
}

void setspritepos(struct amigavideo_staticdata *csd, WORD x, WORD y)
{
    UWORD ctl, pos;

    csd->spritex = x;
    csd->spritey = y;
    if (!csd->sprite || csd->sprite_height == 0)
        return;

    x += csd->sprite_offset_x << csd->res;
    x <<= (2 - csd->res); // convert x to shres coordinates
    x += (csd->startx - 1) << 2; // display left edge offset
 
    if (csd->interlace)
        y /= 2; // y is always in nonlaced
    y += csd->starty;
    y += csd->sprite_offset_y;

    pos = (y << 8) | (x >> 3);
    ctl = ((y + csd->sprite_height) << 8);
    ctl |= ((y >> 8) << 2) | (((y + csd->sprite_height) >> 8) << 1) | ((x >> 2) & 1) | ((x & 3) << 3);
    csd->spritepos = pos;
    csd->spritectl = ctl;
}

void setspritevisible(struct amigavideo_staticdata *csd, BOOL visible)
{
    csd->cursorvisible = visible;
    if (visible) {
        if (csd->copper1_spritept) {
            UWORD *p = csd->sprite;
            setfmode(csd);
            csd->copper1_spritept[0] = (UWORD)(((ULONG)p) >> 16);
            csd->copper1_spritept[2] = (UWORD)(((ULONG)p) >> 0);
}
    } else {
        setnullsprite(csd);
    }
}

BOOL setcolors(struct amigavideo_staticdata *csd, struct pHidd_BitMap_SetColors *msg, BOOL visible)
{
    UWORD i, j;
    if (msg->firstColor + msg->numColors > csd->max_colors)
        return FALSE;
    j = 0;
    for (i = msg->firstColor; j < msg->numColors; i++, j++) {
        UBYTE red, green, blue;
        red   = msg->colors[j].red   >> 8;
        green = msg->colors[j].green >> 8;
        blue  = msg->colors[j].blue  >> 8;
        csd->palette[i * 3 + 0] = red;
        csd->palette[i * 3 + 1] = green;
        csd->palette[i * 3 + 2] = blue;
        //bug("%d: %02x %02x %02x\n", i, red, green, blue);
    }
    if (visible)
        setcoppercolors(csd);
    return TRUE;
}
void setscroll(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    csd->updatescroll = bm;
}

/* Convert Z flag to normal C-style return variable. Fun. */
UBYTE bltnode_wrapper(void)
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

static AROS_INTH1(gfx_vblank, struct amigavideo_staticdata*, csd)
{ 
    AROS_INTFUNC_INIT

    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    BOOL lof = (custom->vposr & 0x8000) != 0;

    if (csd->interlace) {
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

    if (csd->updatescroll) {
        setcopperscroll(csd, csd->updatescroll);
        csd->updatescroll = NULL;
    }

    if (bqvar & BQ_BEAMSYNC)
        bqvar |= BQ_MISSED;

    return FALSE;
        
    AROS_INTFUNC_EXIT
}

void initcustom(struct amigavideo_staticdata *csd)
{
    UBYTE i;
    UWORD *c;
    UWORD vposr, val;
    struct GfxBase *GfxBase;
    struct Library *OOPBase;
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    volatile struct CIA *ciab = (struct CIA*)0xbfd000;

    D(bug("[AmigaVideo] %s()\n", __func__));

    /* Reset audio registers to values that help emulation
     * if some program enables audio DMA without setting period
     * or length. Very high period emulation is very CPU intensive.
     */
    for (i = 0; i < 4; i++) {
        custom->aud[i].ac_vol = 0;
        custom->aud[i].ac_per = 100;
        custom->aud[i].ac_len = 1000;
    }

    /* csd->cs_OOPBase was already set up.
     * See amigavideo.conf's 'oopbase_field' config
     */
    OOPBase = csd->cs_OOPBase;
    csd->cs_HiddBitMapBase = OOP_GetMethodID(IID_Hidd_BitMap, 0);
    csd->cs_HiddGfxBase = OOP_GetMethodID(IID_Hidd_Gfx, 0);

    csd->cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!csd->cs_UtilityBase)
        Alert(AT_DeadEnd | AN_Hidd | AG_OpenLib | AO_UtilityLib);
    csd->cs_GfxBase = TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    if (!csd->cs_GfxBase)
        Alert(AT_DeadEnd | AN_Hidd | AG_OpenLib | AO_GraphicsLib);
    GfxBase = ((struct GfxBase *)csd->cs_GfxBase);
    GfxBase->cia = OpenResource("ciab.resource");

    /* Reset now we have the bases */
    resetcustom(csd);
    resetsprite(csd);

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
    GfxBase->NormalDisplayRows = (GfxBase->DisplayFlags & NTSC) ? 200 : 256;
    GfxBase->MaxDisplayColumn = 640;
    GfxBase->MaxDisplayRow = (GfxBase->DisplayFlags & NTSC) ? 200 : 256;

    csd->startx = 0x81;
    csd->starty = 0x28;

    vposr = custom->vposr & 0x7f00;
    csd->aga = vposr >= 0x2200;
    csd->ecs_agnus = vposr >= 0x2000;
    val = custom->deniseid;
    custom->deniseid = custom->dmaconr;;
    if (val == custom->deniseid) {
        custom->deniseid = custom->dmaconr ^ 0x8000;
        if (val == custom->deniseid) {
            if ((val & (2 + 8)) == 8)
                csd->ecs_denise = TRUE;
        }
    }
    csd->max_colors = csd->aga ? 256 : 32;
    csd->palette = AllocVec(csd->max_colors * 3, MEMF_CLEAR);
    csd->copper1 = AllocVec(22 * 2 * sizeof(WORD), MEMF_CLEAR | MEMF_CHIP);
    csd->sprite_null = AllocMem(2 * 8, MEMF_CLEAR | MEMF_CHIP);
    csd->sprite_res = 0; /* lores */
    c = csd->copper1;
    for (i = 0; i < 8; i++) {
        *c++ = 0x0120 + i * 4;
        if (i == 0)
            csd->copper1_spritept = c;
        *c++ = (UWORD)(((ULONG)csd->sprite_null) >> 16);
        *c++ = 0x0122 + i * 4;
        *c++ = (UWORD)(((ULONG)csd->sprite_null) >> 0);
    }
    *c++ = 0x0c03;
    *c++ = 0xfffe;
    *c++ = 0x008a;
    *c++ = 0x0000;
    csd->copper2_backup = c;
    *c++ = 0xffff;
    *c++ = 0xfffe;
    custom->cop1lc = (ULONG)csd->copper1;
    custom->cop2lc = (ULONG)csd->copper2_backup;
    custom->dmacon = 0x8000 | 0x0080 | 0x0040 | 0x0020;
    
    GfxBase->copinit = (struct copinit*)csd->copper1;

    D(bug("Copperlist0 %p\n", csd->copper1));

}

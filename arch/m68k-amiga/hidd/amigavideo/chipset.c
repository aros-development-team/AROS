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

#define DEBUG 0
#include <aros/debug.h>

#define BPLCONMASK 0x8a55

static const UBYTE fetchunits[] = { 3,3,3,0, 4,3,3,0, 5,4,3,0 };
static const UBYTE fm_maxplanes[] = { 3,2,1,0, 3,3,2,0, 3,3,3,0 };

/* reset to OCS defaults */
void resetcustom(struct amigavideo_staticdata *data)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;

    GfxBase->system_bplcon0 &= ~BPLCONMASK;
    GfxBase->system_bplcon0 |= 0x0200;
    custom->fmode = 0x0000;
    custom->bplcon0 = GfxBase->system_bplcon0;
    custom->bplcon1 = 0x0000;
    custom->bplcon2 = 0x0024;
    custom->bplcon3 = 0x0c00;
    custom->bplcon4 = 0x0011;
    custom->vposw = 0x8000;
    custom->color[0] = 0x0444;

    // Use AGA modes and create AGA copperlists only if AGA is "enabled"
    data->aga_enabled = data->aga && GfxBase->ChipRevBits0 == SETCHIPREV_AA;
}

static void waitvblank(struct amigavideo_staticdata *data)
{
    // ugly busy loop for now..
    UWORD fc = data->framecounter;
    while (fc == data->framecounter);
}
 
static void setnullsprite(struct amigavideo_staticdata *data)
{
    if (data->copper1_spritept) {
        UWORD *p = data->sprite_null;
        data->copper1_spritept[0] = (UWORD)(((ULONG)p) >> 16);
        data->copper1_spritept[2] = (UWORD)(((ULONG)p) >> 0);
    }
 }
 
void resetsprite(struct amigavideo_staticdata *data)
{
    UWORD *sprite = data->sprite;
    setnullsprite(data);
    data->sprite = NULL;
    FreeMem(sprite, data->spritedatasize);
    data->sprite_width = data->sprite_height = 0;
}

static void setfmode(struct amigavideo_staticdata *data)
{
    UWORD fmode;
    fmode  =  data->fmode_bpl == 2 ? 3 : data->fmode_bpl;
    fmode |= (data->fmode_spr == 2 ? 3 : data->fmode_spr) << 2;
    if (data->copper2.copper2_fmode) {
        *data->copper2.copper2_fmode = fmode;
        if (data->interlace)
            *data->copper2i.copper2_fmode = fmode;
    }
}

static void setcoppercolors(struct amigavideo_staticdata *data)
{
    UWORD i;
 
    if (!data->copper2.copper2_palette)
        return;
    if (data->aga && data->aga_enabled) {
        UWORD off = 1;
        for (i = 0; i < data->use_colors; i++) {
            UWORD vallo, valhi;
            UBYTE r = data->palette[i * 3 + 0];
            UBYTE g = data->palette[i * 3 + 1];
            UBYTE b = data->palette[i * 3 + 2];
            if ((i & 31) == 0)
                off += 2;
            valhi = ((r & 0xf0) << 4) | ((g & 0xf0)) | ((b & 0xf0) >> 4);
            vallo = ((r & 0x0f) << 8) | ((g & 0x0f) << 4) | ((b & 0x0f));
            data->copper2.copper2_palette[i * 2 + off] = valhi;
            data->copper2.copper2_palette_aga_lo[i * 2 + off] = vallo;
            if (data->interlace) {
                data->copper2i.copper2_palette[i * 2 + off] = valhi;
                data->copper2i.copper2_palette_aga_lo[i * 2 + off] = vallo;
            }	
        }   
    } else if (data->res == 2 && !data->aga) {
        /* ECS "scrambled" superhires */
        for (i = 0; i < data->use_colors; i++) {
            UBYTE offset = i < 16 ? 0 : 16;
            UBYTE c1 = (i & 3) + offset;
            UBYTE c2 = ((i >> 2) & 3) + offset;
            UWORD val1 = ((data->palette[c1 * 3 + 0] >> 4) << 8) | ((data->palette[c1 * 3 + 1] >> 4) << 4) | ((data->palette[c1 * 3 + 2] >> 4) << 0);
            UWORD val2 = ((data->palette[c2 * 3 + 0] >> 4) << 8) | ((data->palette[c2 * 3 + 1] >> 4) << 4) | ((data->palette[c2 * 3 + 2] >> 4) << 0);
            UWORD val = (val1 & 0xccc) | ((val2 & 0xccc) >> 2);
            data->copper2.copper2_palette[i * 2 + 1] = val;
            if (data->interlace)
                data->copper2i.copper2_palette[i * 2 + 1] = val;
        }
        
    } else {
        for (i = 0; i < data->use_colors; i++) {
            UWORD val = ((data->palette[i * 3 + 0] >> 4) << 8) | ((data->palette[i * 3 + 1] >> 4) << 4) | ((data->palette[i * 3 + 2] >> 4) << 0);
            data->copper2.copper2_palette[i * 2 + 1] = val;
            if (data->interlace)
                data->copper2i.copper2_palette[i * 2 + 1] = val;
        }
    }
}

static void setpalntsc(struct amigavideo_staticdata *data, ULONG modeid)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;

    data->palmode = (GfxBase->DisplayFlags & NTSC) == 0;
    if (!data->ecs_agnus)	
        return;
    if ((modeid & MONITOR_ID_MASK) == PAL_MONITOR_ID) {
        custom->beamcon0 = 0x0020;
        data->palmode = TRUE;
    } else if ((modeid & MONITOR_ID_MASK) == NTSC_MONITOR_ID) {
        custom->beamcon0 = 0x0000;
        data->palmode = FALSE;
    } else {
        custom->beamcon0 = (GfxBase->DisplayFlags & NTSC) ? 0x0000 : 0x0020;
    }
}

void resetmode(struct amigavideo_staticdata *data)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;

    D(bug("resetmode\n"));

    data->disp = NULL;

    custom->dmacon = 0x0100;
    setpalntsc(data, 0);

    custom->cop2lc = (ULONG)data->copper2_backup;
    custom->copjmp2 = 0;

    waitvblank(data);

    FreeVec(data->copper2.copper2);
    data->copper2.copper2 = NULL;
    FreeVec(data->copper2i.copper2);
    data->copper2i.copper2 = NULL;

    GfxBase->LOFlist = GfxBase->SHFlist = data->copper2_backup;

    resetcustom(data);

    data->depth = 0;
}

/* Use nominal screen height. Overscan is not supported yet. */
static WORD limitheight(struct amigavideo_staticdata *data, WORD y, BOOL lace, BOOL maxlimit)
{
    if (lace)
        y /= 2;
    if (data->palmode) {
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

static void setcopperscroll2(struct amigavideo_staticdata *data, struct amigabm_data *bm, struct copper2data *c2d, BOOL odd)
{
    UWORD *copptr = c2d->copper2_scroll, *copbpl;
    WORD xscroll, yscroll;
    WORD x, y;
    WORD ystart, yend, i, xdelay;
    WORD xmaxscroll,  modulo, ddfstrt, fmodewidth, minearly;
    LONG offset;
    
    fmodewidth = 16 << data->fmode_bpl;
    x = bm->leftedge;
    y = data->starty + (bm->topedge >> data->interlace);
    
    yscroll = 0;
    if (y < 10) {
        yscroll = y - 10;
        y = 10;
    }

    xmaxscroll = 1 << (1 + data->fmode_bpl);
    xdelay = x & (fmodewidth - 1);
    xscroll = -x;
    
    yend = y + (bm->displayheight >> data->interlace);
    yend = limitheight(data, yend, FALSE, TRUE);
    ystart = y - data->extralines;
        
    modulo = (data->interlace ? bm->bytesperrow : 0) + data->modulo;
    ddfstrt = data->ddfstrt;

    offset = ((xscroll + (xmaxscroll << 3) - 1) >> 3) & ~(xmaxscroll - 1);
    offset -= (yscroll * bm->bytesperrow) << (data->interlace ? 1 : 0);

    minearly = 1 << fetchunits[data->fmode_bpl * 4 + data->res];
    if (xdelay) {
        ddfstrt -= minearly;
        modulo -= (minearly << data->res) / 4;
        offset -= (minearly << data->res) / 4;
    }

    copptr[1] = (y << 8) | (data->startx); //(y << 8) + (x + 1);
    copptr[3] = (yend << 8) | ((data->startx + 0x140) & 0xff); //((y + (bm->rows >> data->interlace)) << 8) + ((x + 1 + (bm->width >> data->res)) & 0x00ff);
    copptr[5] = ((y >> 8) & 7) | (((yend >> 8) & 7) << 8) | 0x2000;

    copbpl = c2d->copper2_bpl;
    for (i = 0; i < bm->depth; i++) {
        ULONG pptr = (ULONG)(bm->pbm->Planes[data->bploffsets[i]]);
        if (data->interlace && odd)
            pptr += bm->bytesperrow;
        pptr += offset;
        copbpl[1] = (UWORD)(pptr >> 16);
        copbpl[3] = (UWORD)(pptr >> 0);
        copbpl += 4;
    }

    xdelay <<= 2 - data->res;
    copptr[11] =
          (((xdelay >> 2) & 0x0f) << 0) | (((xdelay >> 2) & 0x0f) << 4)
        | ((xdelay >> 6) << 10) | ((xdelay >> 6) << 14)
        | ((xdelay & 3) << 8) | ((xdelay & 3) << 12);

    copptr[7] = ddfstrt;
    copptr[9] = data->ddfstop;
    copptr[13] = modulo;
    copptr[15] = modulo;

    yend = y + bm->displayheight + yscroll;
    yend = limitheight(data, yend, FALSE, TRUE);
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

static void setcopperscroll(struct amigavideo_staticdata *data, struct amigabm_data *bm)
{
    setcopperscroll2(data, bm, &data->copper2, FALSE);
    if (data->interlace)
        setcopperscroll2(data, bm, &data->copper2i, TRUE);
}

static UWORD get_copper_list_length(struct amigavideo_staticdata *data, UBYTE depth)
{
    UWORD v;

    if (data->aga && data->aga_enabled) {
        v = 1000 + ((1 << depth) + 1 + (1 << depth) / 32 + 1) * 2;
    } else {
        v = 1000;
    }
    return v * 2;
}

static void createcopperlist(struct amigavideo_staticdata *data, struct amigabm_data *bm, struct copper2data *c2d, BOOL lace)
{
    struct GfxBase *GfxBase = (struct GfxBase *)data->cs_GfxBase;
    volatile WORD *system_bplcon0 = (volatile WORD *)&GfxBase->system_bplcon0;
    UWORD *c;
    UWORD i;
    UWORD bplcon0, bplcon0_res;
    ULONG pptr;

    c = c2d->copper2;
    D(bug("Copperlist%d %p\n", lace ? 2 : 1, c));

    bplcon0_res = *system_bplcon0 & ~BPLCONMASK;

    if (data->res == 1)
         bplcon0_res |= 0x8000;
    else if (data->res == 2)
        bplcon0_res |= 0x0040;
    else
        bplcon0_res = 0;

    data->bplcon0_null = 0x0201 | (data->interlace ? 4 : 0) | bplcon0_res;
    data->bplcon3 = ((data->sprite_res + 1) << 6) | 2; // spriteres + bordersprite

    *c++ = 0x01fe;
    *c++ = 0xfffe;
    *c++ = 0xffff;
    *c++ = 0xfffe;

    *c++ = 0x0100;
    *c++ = data->bplcon0_null;

    c2d->copper2_bpl = c;
    for (i = 0; i < bm->depth; i++) {
        pptr = (ULONG)(bm->pbm->Planes[data->bploffsets[i]]);
        if (lace)
            pptr += bm->bytesperrow;
        *c++ = 0xe0 + i * 4;
        *c++ = (UWORD)(pptr >> 16);
        *c++ = 0xe2 + i * 4;
        *c++ = (UWORD)(pptr >> 0);
    }

    data->use_colors = 1 << bm->depth;
    // need to update sprite colors
    if (data->use_colors < 16 + 4)
        data->use_colors = 16 + 4;
    if (data->res == 2 && !data->aga)
        data->use_colors = 32; /* ECS "scrambled" superhires */
    
    if (data->use_colors > 32 && (data->modeid & EXTRAHALFBRITE_KEY))
        data->use_colors = 32;
    if (data->modeid & HAM_KEY) {
        if (bm->depth <= 6)
            data->use_colors = 16 + 4;
        else
            data->use_colors = 64;
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
    *c++ = 0x0024 | ((data->aga && !(data->modeid & EXTRAHALFBRITE_KEY)) ? 0x0200 : 0);

    c2d->copper2_fmode = NULL;
    if (data->aga && data->aga_enabled) {
        *c++ = 0x010c;
        *c++ = 0x0011;
        *c++ = 0x01fc;
        c2d->copper2_fmode = c;
        *c++ = 0;
    }

    bplcon0 = data->bplcon0_null;
    if (bm->depth > 7)
        bplcon0 |= 0x0010;
    else
        bplcon0 |= bm->depth << 12;
    if (data->modeid & HAM_KEY)
        bplcon0 |= 0x0800;

    *system_bplcon0 = (*system_bplcon0 & ~BPLCONMASK) | bplcon0;

    c2d->copper2_palette = c;
    if (data->aga && data->aga_enabled) {
        // hi
        for (i = 0; i < data->use_colors; i++) {
            UBYTE agac = i & 31;
            if (agac == 0) {
                *c++ = 0x106;
                *c++ = data->bplcon3 | ((i / 32) << 13);
            }
            *c++ = 0x180 + agac * 2;
            *c++ = 0x000;
        }
        c2d->copper2_palette_aga_lo = c;
        // lo
        for (i = 0; i < data->use_colors; i++) {
            UBYTE agac = i & 31;
            if (agac == 0) {
                *c++ = 0x106;
                *c++ = data->bplcon3 | ((i / 32) << 13) | 0x0200;
            }
            *c++ = 0x180 + agac * 2;
            *c++ = 0x000;
        }
        *c++ = 0x106;
        *c++ = data->bplcon3;
    } else {
        // ocs/ecs
        for (i = 0; i < data->use_colors; i++) {
            *c++ = 0x180 + i * 2;
            *c++ = 0x000;
        }
    }

    data->extralines = (c - c2d->copper2) / 112 + 1;

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
    *c++ = data->bplcon0_null;

   if (data->interlace) {
        ULONG nextptr = (ULONG)(lace ? data->copper2.copper2 : data->copper2i.copper2);
        *c++ = 0x0084;
        *c++ = (UWORD)(nextptr >> 16);
        *c++ = 0x0086;
        *c++ = (UWORD)(nextptr >> 0);
    }
    *c++ = 0xffff;
    *c++ = 0xfffe;

}

BOOL setbitmap(struct amigavideo_staticdata *data, struct amigabm_data *bm)
{
    data->width = bm->width;
    data->height = data->interlace ? (bm->height + 1) / 2 : bm->height;
    data->modulo = bm->bytesperrow - data->modulopre / (4 >> data->res);
    data->modulo &= ~((2 << data->fmode_bpl) - 1);
    data->updatescroll = bm;
    data->depth = bm->depth;
    setcopperscroll(data, bm);

    D(bug("setbitmap bm=%x mode=%08x w=%d h=%d d=%d bpr=%d\n",
        bm, data->modeid, bm->width, bm->height, bm->depth, bm->bytesperrow));
        return TRUE;
}

BOOL setmode(struct amigavideo_staticdata *data, struct amigabm_data *bm)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;
    UWORD ddfstrt, ddfstop;
    UBYTE fetchunit, maxplanes;
    UWORD bplwidth, viewwidth;
    UBYTE i;

    if (data->disp == bm)
        return TRUE;

    resetmode(data);

    data->res = 0;
    if ((data->modeid & SUPER_KEY) == SUPER_KEY)
        data->res = 2;
    else if ((data->modeid & SUPER_KEY) == HIRES_KEY)
        data->res = 1;
    data->interlace = (data->modeid & LORESLACE_KEY) ? 1 : 0;
    data->fmode_bpl = data->aga && data->aga_enabled ? 2 : 0;

    fetchunit = fetchunits[data->fmode_bpl * 4 + data->res];
    maxplanes = fm_maxplanes[data->fmode_bpl * 4 + data->res];

    D(bug("res %d fmode %d depth %d maxplanes %d aga %d agae %d\n",
        data->res, data->fmode_bpl, bm->depth, maxplanes, data->aga, data->aga_enabled));

    if (bm->depth > (1 << maxplanes)) {
        if (data->aga && !data->aga_enabled) {
            // Enable AGA if requesting AGA only mode.
            // This is a compatibility hack because our display
            // database currently contains all AGA modes even if AGA
            // is "disabled".
            GfxBase->ChipRevBits0 = SETCHIPREV_AA;
            data->aga_enabled = TRUE;
            data->fmode_bpl = data->aga && data->aga_enabled ? 2 : 0;
            fetchunit = fetchunits[data->fmode_bpl * 4 + data->res];
            maxplanes = fm_maxplanes[data->fmode_bpl * 4 + data->res];
        }
        if (bm->depth > (1 << maxplanes))
            return FALSE;
    }

    viewwidth = bm->width;
    // use nominal width for now
    if ((viewwidth << data->res) > 320)
        viewwidth = 320 << data->res;

    D(bug("setmode bm=%x mode=%08x w=%d h=%d d=%d bpr=%d fu=%d\n",
        bm, data->modeid, bm->width, bm->height, bm->depth, bm->bytesperrow, fetchunit));
    
    bplwidth = viewwidth >> (data->res + 1);
    ddfstrt = (data->startx / 2) & ~((1 << fetchunit) - 1);
    ddfstop = ddfstrt + ((bplwidth + ((1 << fetchunit) - 1) - 2 * (1 << fetchunit)) & ~((1 << fetchunit) - 1));
    data->modulopre = ddfstop + 2 * (1 << fetchunit) - ddfstrt;
    ddfstrt -= 1 << maxplanes;
    data->ddfstrt = ddfstrt;
    data->ddfstop = ddfstop;

    for (i = 0; i < 8; i++)
        data->bploffsets[i] = i;
    if ((data->modeid & HAM_KEY) && bm->depth > 6) {
        data->bploffsets[0] = 6;
        data->bploffsets[1] = 7;
        for (i = 0; i < 6; i++)
            data->bploffsets[i + 2] = i;
    }

    data->copper2.copper2 = AllocVec(get_copper_list_length(data, bm->depth), MEMF_CLEAR | MEMF_CHIP);
    if (data->interlace)
        data->copper2i.copper2 = AllocVec(get_copper_list_length(data, bm->depth), MEMF_CLEAR | MEMF_CHIP);
    createcopperlist(data, bm, &data->copper2, FALSE);
    if (data->interlace) {
        createcopperlist(data, bm, &data->copper2i, TRUE);
    }
 
    setfmode(data);
    setpalntsc(data, data->modeid);
    custom->bplcon0 = data->bplcon0_null;

    bm->displaywidth = viewwidth;
    bm->displayheight = limitheight(data, bm->height, data->interlace, FALSE);

    setbitmap(data, bm);

        GfxBase->LOFlist = data->copper2.copper2;
        GfxBase->SHFlist = data->interlace ? data->copper2i.copper2 : data->copper2.copper2;
        custom->dmacon = 0x8100;

    setcoppercolors(data);
    setspritepos(data, data->spritex, data->spritey);

    data->disp = bm;
    return 1;
 }

BOOL setsprite(struct amigavideo_staticdata *data, WORD width, WORD height, struct pHidd_Gfx_SetCursorShape *shape)
{
    OOP_MethodID HiddBitMapBase = data->cs_HiddBitMapBase;
    UWORD fetchsize;
    UWORD bitmapwidth = width;
    UWORD y, *p;
    
    if (data->aga && data->aga_enabled && width > 16)
        data->fmode_spr = 2;
    else
        data->fmode_spr = 0;
    fetchsize = 2 << data->fmode_spr;
    width = 16 << data->fmode_spr;

    if (width != data->sprite_width || height != data->sprite_height) {
        resetsprite(data);
        data->spritedatasize = fetchsize * 2 + fetchsize * height * 2 + fetchsize * 2;
        data->sprite = AllocMem(data->spritedatasize, MEMF_CHIP | MEMF_CLEAR);
        if (!data->sprite)
            return FALSE;
        data->sprite_width = width;
        data->sprite_height = height;
        data->sprite_offset_x = shape->xoffset;
        data->sprite_offset_y = shape->yoffset;
    }
    p = data->sprite;
    p += fetchsize;
    for(y = 0; y < height; y++) {
        UWORD xx, xxx, x;
        for (xx = 0, xxx = 0; xx < width; xx += 16, xxx++) {
            UWORD pix1 = 0, pix2 = 0;
            for(x = 0; x < 16; x++) {
                UBYTE c = 0;
                if (xx + x < bitmapwidth)
                    c = HIDD_BM_GetPixel(shape->shape, xx + x, y);
#if 0
                /* Sprite alignment grid */
                if (xx + x == 0 || xx + x == width - 1 || y == 0 || y == height - 1) {
                    c = 2;
                } else if (0) {
                    c = 0;
                }
#endif
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
    setspritepos(data, data->spritex, data->spritey);
    setspritevisible(data, data->cursorvisible);
    return TRUE;
}

void setspritepos(struct amigavideo_staticdata *data, WORD x, WORD y)
{
    UWORD ctl, pos;

    data->spritex = x;
    data->spritey = y;
    if (!data->sprite || data->sprite_height == 0)
        return;

    x += data->sprite_offset_x << data->res;
    x <<= (2 - data->res); // convert x to shres coordinates
    x += (data->startx - 1) << 2; // display left edge offset
 
    if (data->interlace)
        y /= 2; // y is always in nonlaced
    y += data->starty;
    y += data->sprite_offset_y;

    pos = (y << 8) | (x >> 3);
    ctl = ((y + data->sprite_height) << 8);
    ctl |= ((y >> 8) << 2) | (((y + data->sprite_height) >> 8) << 1) | ((x >> 2) & 1) | ((x & 3) << 3);
    data->spritepos = pos;
    data->spritectl = ctl;
}

void setspritevisible(struct amigavideo_staticdata *data, BOOL visible)
{
    data->cursorvisible = visible;
    if (visible) {
        if (data->copper1_spritept) {
            UWORD *p = data->sprite;
            setfmode(data);
            data->copper1_spritept[0] = (UWORD)(((ULONG)p) >> 16);
            data->copper1_spritept[2] = (UWORD)(((ULONG)p) >> 0);
}
    } else {
        setnullsprite(data);
    }
}

BOOL setcolors(struct amigavideo_staticdata *data, struct pHidd_BitMap_SetColors *msg, BOOL visible)
{
    UWORD i, j;
    if (msg->firstColor + msg->numColors > data->max_colors)
        return FALSE;
    j = 0;
    for (i = msg->firstColor; j < msg->numColors; i++, j++) {
        UBYTE red, green, blue;
        red   = msg->colors[j].red   >> 8;
        green = msg->colors[j].green >> 8;
        blue  = msg->colors[j].blue  >> 8;
        data->palette[i * 3 + 0] = red;
        data->palette[i * 3 + 1] = green;
        data->palette[i * 3 + 2] = blue;
        //bug("%d: %02x %02x %02x\n", i, red, green, blue);
    }
    if (visible)
        setcoppercolors(data);
    return TRUE;
}
void setscroll(struct amigavideo_staticdata *data, struct amigabm_data *bm)
{
    data->updatescroll = bm;
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

static AROS_INTH1(gfx_beamsync, struct amigavideo_staticdata*, data)
{ 
    AROS_INTFUNC_INIT

    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;

    if (bqvar & BQ_BEAMSYNCWAITING) {
        /* We only need to trigger blitter interrupt */
        volatile struct Custom *custom = (struct Custom*)0xdff000;
        custom->intreq = INTF_SETCLR | INTF_BLIT;
    }

    return FALSE;
        
    AROS_INTFUNC_EXIT
}

static AROS_INTH1(gfx_vblank, struct amigavideo_staticdata*, data)
{ 
    AROS_INTFUNC_INIT

    struct GfxBase *GfxBase = (APTR)data->cs_GfxBase;
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    BOOL lof = (custom->vposr & 0x8000) != 0;

    if (data->interlace) {
        custom->cop2lc = (ULONG)(lof ? GfxBase->LOFlist : GfxBase->SHFlist);
    } else {
        custom->cop2lc = (ULONG)GfxBase->LOFlist;
        /* We may be in SHF mode after switching interlace off. Fix it here. */
        if (!lof)
            custom->vposw = custom->vposr | 0x8000;
    }

    data->framecounter++;
    if (data->sprite) {
        UWORD *p = data->sprite;
        p[0] = data->spritepos;
        p[1 << data->fmode_spr] = data->spritectl;
    }

    if (data->updatescroll) {
        setcopperscroll(data, data->updatescroll);
        data->updatescroll = NULL;
    }

    if (bqvar & BQ_BEAMSYNC)
        bqvar |= BQ_MISSED;

    return FALSE;
        
    AROS_INTFUNC_EXIT
}

void initcustom(struct amigavideo_staticdata *data)
{
    UBYTE i;
    UWORD *c;
    UWORD vposr, val;
    struct GfxBase *GfxBase;
    struct Library *OOPBase;
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    volatile struct CIA *ciab = (struct CIA*)0xbfd000;

    /* Reset audio registers to values that help emulation
     * if some program enables audio DMA without setting period
     * or length. Very high period emulation is very CPU intensive.
     */
    for (i = 0; i < 4; i++) {
        custom->aud[i].ac_vol = 0;
        custom->aud[i].ac_per = 100;
        custom->aud[i].ac_len = 1000;
    }

    resetcustom(data);
    resetsprite(data);

    /* data->cs_OOPBase was already set up.
     * See amigavideo.conf's 'oopbase_field' config
     */
    OOPBase = data->cs_OOPBase;
    data->cs_HiddBitMapBase = OOP_GetMethodID(IID_Hidd_BitMap, 0);
    data->cs_HiddGfxBase = OOP_GetMethodID(IID_Hidd_Gfx, 0);

    data->cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!data->cs_UtilityBase)
        Alert(AT_DeadEnd | AN_Hidd | AG_OpenLib | AO_UtilityLib);
    data->cs_GfxBase = TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    if (!data->cs_GfxBase)
        Alert(AT_DeadEnd | AN_Hidd | AG_OpenLib | AO_GraphicsLib);
    GfxBase = ((struct GfxBase *)data->cs_GfxBase);
    GfxBase->cia = OpenResource("ciab.resource");

    data->inter.is_Code         = (APTR)gfx_vblank;
    data->inter.is_Data         = data;
    data->inter.is_Node.ln_Name = "GFX VBlank server";
    data->inter.is_Node.ln_Pri  = 25;
    data->inter.is_Node.ln_Type = NT_INTERRUPT;
    AddIntServer(INTB_VERTB, &data->inter);

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
    GfxBase->timsrv.is_Data = data;
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

    data->startx = 0x81;
    data->starty = 0x28;

    vposr = custom->vposr & 0x7f00;
    data->aga = vposr >= 0x2200;
    data->ecs_agnus = vposr >= 0x2000;
    val = custom->deniseid;
    custom->deniseid = custom->dmaconr;;
    if (val == custom->deniseid) {
        custom->deniseid = custom->dmaconr ^ 0x8000;
        if (val == custom->deniseid) {
            if ((val & (2 + 8)) == 8)
                data->ecs_denise = TRUE;
        }
    }
    data->max_colors = data->aga ? 256 : 32;
    data->palette = AllocVec(data->max_colors * 3, MEMF_CLEAR);
    data->copper1 = AllocVec(22 * 2 * sizeof(WORD), MEMF_CLEAR | MEMF_CHIP);
    data->sprite_null = AllocMem(2 * 8, MEMF_CLEAR | MEMF_CHIP);
    data->sprite_res = 0; /* lores */
    c = data->copper1;
    for (i = 0; i < 8; i++) {
        *c++ = 0x0120 + i * 4;
        if (i == 0)
            data->copper1_spritept = c;
        *c++ = (UWORD)(((ULONG)data->sprite_null) >> 16);
        *c++ = 0x0122 + i * 4;
        *c++ = (UWORD)(((ULONG)data->sprite_null) >> 0);
    }
    *c++ = 0x0c03;
    *c++ = 0xfffe;
    *c++ = 0x008a;
    *c++ = 0x0000;
    data->copper2_backup = c;
    *c++ = 0xffff;
    *c++ = 0xfffe;
    custom->cop1lc = (ULONG)data->copper1;
    custom->cop2lc = (ULONG)data->copper2_backup;
    custom->dmacon = 0x8000 | 0x0080 | 0x0040 | 0x0020;
    
    GfxBase->copinit = (struct copinit*)data->copper1;

    D(bug("Copperlist0 %p\n", data->copper1));

}

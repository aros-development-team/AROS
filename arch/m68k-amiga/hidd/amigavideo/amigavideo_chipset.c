/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cia.h>

#include <exec/libraries.h>
#include <exec/lists.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hardware/cia.h>
#include <hidd/gfx.h>
#include <graphics/modeid.h>

#include "amigavideo_hidd.h"
#include "amigavideo_bitmap.h"
#include "chipset.h"

#include <aros/debug.h>

#define BPLCONMASKFULL  0xFA55
#define BPLCONMASK      0x8A55

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

    GfxBase->system_bplcon0 &= ~BPLCONMASKFULL;
    GfxBase->system_bplcon0 |= 0x0200;
    D(bug("[AmigaVideo] %s: system_bplcon0 = %04x\n", __func__, GfxBase->system_bplcon0));

    custom->fmode = 0x0000;
    custom->bplcon0 = GfxBase->system_bplcon0;
    csd->bplcon0_null = GfxBase->system_bplcon0 | 0x1;
    custom->bplcon1 = 0x0000;
    custom->bplcon2 = 0x0024;
    csd->bplcon3 = 0x0c00;          /* set PF2OFx to default offset value 8 (plane 3 affected) */
    custom->bplcon3 = csd->bplcon3;
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

void setfmode(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    UWORD fmode;
    fmode  =  csd->fmode_bpl == 2 ? 3 : csd->fmode_bpl;
    fmode |= (csd->fmode_spr == 2 ? 3 : csd->fmode_spr) << 2;
    if (bm && bm->copld.copper2_fmode) {
        *bm->copld.copper2_fmode = fmode;
        if (bm->interlace)
            *bm->copsd.copper2_fmode = fmode;
    }
}

void setcoppercolors(struct amigavideo_staticdata *csd, struct amigabm_data *bm, UBYTE *palette)
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

    UWORD i;

    if (csd->aga && csd->aga_enabled) {
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
            if (bm->interlace) {
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
            if (bm->interlace)
                c2di->copper2_palette[i * 2 + 1] = val;
        }
        
    } else {
        for (i = 0; i < bm->use_colors; i++) {
            UWORD val = ((palette[i * 3 + 0] >> 4) << 8) | ((palette[i * 3 + 1] >> 4) << 4) | ((palette[i * 3 + 2] >> 4) << 0);
            c2d->copper2_palette[i * 2 + 1] = val;
            if (bm->interlace)
                c2di->copper2_palette[i * 2 + 1] = val;
        }
    }
    D(bug("[AmigaVideo] %s: copper colors set\n", __func__));
}

void resetmode(struct amigavideo_staticdata *csd)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;

    D(bug("[AmigaVideo] %s()\n", __func__));

    custom->dmacon = 0x0100;
    csd->palmode = (GfxBase->DisplayFlags & NTSC) == 0;
    setpalntsc(csd);

    custom->cop2lc = (ULONG)csd->copper2_backup;
    custom->copjmp2 = 0;

    waitvblank(csd);

    GfxBase->LOFlist = GfxBase->SHFlist = csd->copper2_backup;

    resetcustom(csd);
}

static void setcopperscroll2(struct amigavideo_staticdata *csd, struct amigabm_data *bm, struct copper2data *c2d, UWORD *c2, BOOL odd)
{
    UWORD *copptr = c2d->copper2_scroll, *copbpl;
    WORD xscroll, yscroll;
    WORD x, y;
    WORD ystart, yend, i, xdelay;
    WORD xmaxscroll,  modulo, ddfstrt, fmodewidth, minearly;
    LONG offset;

    D(bug("[AmigaVideo] %s(0x%p)\n", __func__, c2));

    fmodewidth = 16 << csd->fmode_bpl;
    x = bm->leftedge;
    y = csd->starty + (bm->topedge >> bm->interlace);

    yscroll = 0;
    if (y < 10) {
        yscroll = y - 10;
        y = 10;
    }

    xmaxscroll = 1 << (1 + csd->fmode_bpl);
    xdelay = x & (fmodewidth - 1);
    xscroll = -x;

    yend = y + (bm->displayheight >> bm->interlace);
    yend = limitheight(csd, yend, FALSE, TRUE);
    ystart = y - c2d->extralines;

    modulo = (bm->interlace ? bm->bytesperrow : 0) + bm->modulo;
    ddfstrt = bm->ddfstrt;

    offset = ((xscroll + (xmaxscroll << 3) - 1) >> 3) & ~(xmaxscroll - 1);
    offset -= (yscroll * bm->bytesperrow) << (bm->interlace ? 1 : 0);

    minearly = 1 << fetchunits[csd->fmode_bpl * 4 + bm->res];
    if (xdelay) {
        ddfstrt -= minearly;
        modulo -= (minearly << bm->res) / 4;
        offset -= (minearly << bm->res) / 4;
    }

    copptr[1] = (y << 8) | (csd->startx); //(y << 8) + (x + 1);
    copptr[3] = (yend << 8) | ((csd->startx + 0x140) & 0xff); //((y + (bm->rows >> bm->interlace)) << 8) + ((x + 1 + (bm->width >> bm->res)) & 0x00ff);
    copptr[5] = ((y >> 8) & 7) | (((yend >> 8) & 7) << 8) | 0x2000;

    copbpl = c2d->copper2_bpl;
    for (i = 0; i < bm->depth; i++) {
        ULONG pptr = (ULONG)(bm->pbm->Planes[bm->bploffsets[i]]);
        if (bm->interlace && odd)
            pptr += bm->bytesperrow;
        pptr += offset;
        copbpl[1] = (UWORD)(pptr >> 16);
        copbpl[3] = (UWORD)(pptr >> 0);
        copbpl += 4;
    }

    xdelay <<= 2 - bm->res;
    copptr[11] =
          (((xdelay >> 2) & 0x0f) << 0) | (((xdelay >> 2) & 0x0f) << 4)
        | ((xdelay >> 6) << 10) | ((xdelay >> 6) << 14)
        | ((xdelay & 3) << 8) | ((xdelay & 3) << 12);

    copptr[7] = ddfstrt;
    copptr[9] = bm->ddfstop;
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

    copptr = c2;
    if (ystart >= 256)
        copptr[0] = 0xffdf;
    else
        copptr[0] = 0x01fe;
    copptr[2] = (ystart << 8) | 0x05;
    copptr = c2d->copper2_bplcon0;
    copptr[-2] = (y << 8) | 0x05;
}

void setcopperscroll(struct amigavideo_staticdata *csd, struct amigabm_data *bm, BOOL interlaced)
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

UWORD *populatebmcopperlist(struct amigavideo_staticdata *csd, struct amigabm_data *bm, struct copper2data *c2d, UWORD *c2, BOOL lace)
{
    struct GfxBase *GfxBase = (struct GfxBase *)csd->cs_GfxBase;
    volatile UWORD *system_bplcon0 = (volatile UWORD *)&GfxBase->system_bplcon0;
    UWORD *c = c2;
    UWORD i;
    UWORD bplcon0, bplcon0_res, bplcon0_null;
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
    bm->bplcon3 = csd->bplcon3 | ((csd->sprite_res + 1) << 6) | 2; // spriteres + bordersprite

    D(bug("[AmigaVideo] %s: bplcon0_null = %04x\n", __func__, bplcon0_null));
    D(bug("[AmigaVideo] %s: bm base bplcon3 = %04x\n", __func__, bm->bplcon3));

    *c++ = 0x01fe;                          // NOP(?)
    *c++ = 0xfffe;
    *c++ = 0xffff;                          // Wait for VBL(?)
    *c++ = 0xfffe;

    *c++ = 0x0100;                          // Push null bplcon0
    *c++ = bplcon0_null;

    c2d->copper2_bpl = c;
    for (i = 0; i < bm->depth; i++) {
        pptr = (ULONG)(bm->pbm->Planes[bm->bploffsets[i]]);
        if (lace)
            pptr += bm->bytesperrow;
        *c++ = 0xe0 + i * 4;                // Push the bitplane registers
        *c++ = (UWORD)(pptr >> 16);
        *c++ = 0xe2 + i * 4;
        *c++ = (UWORD)(pptr >> 0);
    }

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

    c2d->copper2_scroll = c;
    *c++ = 0x008e;                      // Push Display window start
    *c++ = 0;
    *c++ = 0x0090;                      // Push Display window stop
    *c++ = 0;
    *c++ = 0x01e4;                      // Push Display window
    *c++ = 0;
    *c++ = 0x0092;                      // Push Display bitplane data fetch start
    *c++ = 0;
    *c++ = 0x0094;                      // Push Display bitplane data fetch stop
    *c++ = 0;
    *c++ = 0x0102;                      // Push Bitplane scroll control reg
    *c++ = 0;
    *c++ = 0x0108;                      // Push Bitplane modulo (odd planes)
    *c++ = 0;
    *c++ = 0x010a;                      // Push Bitplane modulo (even planes)
    *c++ = 0;
    *c++ = 0x0104;                      // Push Bitplane prio control reg.
    *c++ = 0x0024 | ((csd->aga && !(bm->modeid & EXTRAHALFBRITE_KEY)) ? 0x0200 : 0);

    c2d->copper2_fmode = NULL;
    if (csd->aga && csd->aga_enabled) {
        *c++ = 0x010c;                  // Push (??)
        *c++ = 0x0011;
        *c++ = 0x01fc;                  // Push (??)
        c2d->copper2_fmode = c;
        *c++ = 0;
    }

    // Push the palette registers ...
    c2d->copper2_palette = c;
    if (csd->aga && csd->aga_enabled) {
        // hi
        for (i = 0; i < bm->use_colors; i++) {
            UBYTE agac = i & 31;
            if (agac == 0) {
                *c++ = 0x106;
                *c++ = bm->bplcon3 | ((i / 32) << 13);
            }
            *c++ = 0x180 + agac * 2;
            *c++ = 0x000;
        }
        c2d->copper2_palette_aga_lo = c;
        // lo
        for (i = 0; i < bm->use_colors; i++) {
            UBYTE agac = i & 31;
            if (agac == 0) {
                *c++ = 0x106;
                *c++ = bm->bplcon3 | ((i / 32) << 13) | 0x0200;
            }
            *c++ = 0x180 + agac * 2;
            *c++ = 0x000;
        }
        *c++ = 0x106;
        *c++ = bm->bplcon3;
    } else {
        // ocs/ecs
        for (i = 0; i < bm->use_colors; i++) {
            *c++ = 0x180 + i * 2;
            *c++ = 0x000;
        }
    }

    c2d->extralines = (c - c2) / 112 + 1;

    *c++ = 0xffff;
    *c++ = 0xfffe;

    bplcon0 = bplcon0_null;
    if (bm->depth > 7)
        bplcon0 |= 0x0010;
    else
        bplcon0 |= bm->depth << 12;
    if (bm->modeid & HAM_KEY)
        bplcon0 |= 0x0800;

    D(bug("[AmigaVideo] %s: copper bplcon0 = %04x\n", __func__, bplcon0));

    c2d->copper2_bplcon0 = c;
    *c++ = 0x0100;
    *c++ = bplcon0;                         // Push the screens bplcon0

    *c++ = 0xffff;
    *c++ = 0xfffe;
    *c++ = 0xffff;
    *c++ = 0xfffe;

    *c++ = 0x0100;
    *c++ = bplcon0_null;

    /* store the pointer to the copperlist data tail so it can be adjusted for linking chains */
    c2d->copper2_tail = c;

    return c;
}

VOID setcopperlisttail(struct amigavideo_staticdata *csd, UWORD *copper2tail, UWORD *c2next, BOOL jmp)
{
    UWORD *c = copper2tail;

    D(bug("[AmigaVideo] %s(0x%p)\n", __func__, c2next));
    D(bug("[AmigaVideo] %s: tail @ 0x%p\n", __func__, c));

    if (c2next)
    {
        *c++ = 0x0084;
        *c++ = (UWORD)((IPTR)c2next >> 16);
        *c++ = 0x0086;
        *c++ = (UWORD)((IPTR)c2next >> 0);
    }
    if (jmp)
    {
        D(bug("[AmigaVideo] %s: pushing COPJMP2\n", __func__));
        *c++ = 0x008A;
        *c++ = 0x0000;
    }
    *c++ = 0xffff;
    *c++ = 0xfffe;
}

BOOL setbitmap(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    D(bug("[AmigaVideo] %s()\n", __func__));

    bm->modulo = bm->bytesperrow - bm->modulopre / (4 >> bm->res);
    bm->modulo &= ~((2 << csd->fmode_bpl) - 1);

    csd->updatescroll = bm;
    setcopperscroll(csd, bm, csd->interlaced | bm->interlace);

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
    UWORD bplwidth, viewwidth;
    UBYTE i;

    D(bug("[AmigaVideo] %s(0x%p)\n", __func__, bm));

    csd->fmode_bpl = csd->aga && csd->aga_enabled ? 2 : 0;

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
            GfxBase->ChipRevBits0 = SETCHIPREV_AA;
            csd->aga_enabled = TRUE;
            csd->fmode_bpl = csd->aga && csd->aga_enabled ? 2 : 0;
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

    bplwidth = viewwidth >> (bm->res + 1);
    ddfstrt = (csd->startx / 2) & ~((1 << fetchunit) - 1);
    ddfstop = ddfstrt + ((bplwidth + ((1 << fetchunit) - 1) - 2 * (1 << fetchunit)) & ~((1 << fetchunit) - 1));
    bm->modulopre = ddfstop + 2 * (1 << fetchunit) - ddfstrt;
    ddfstrt -= 1 << maxplanes;
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

    D(bug("[AmigaVideo] %s: done\n", __func__));

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
    struct amigabm_data *data = OOP_INST_DATA(cl, o);
    OOP_Object *bmPFObj = NULL;
    HIDDT_PixelFormat *bmPF;
    IPTR pf, bmcmod;
    UWORD fetchsize;
    UWORD bitmapwidth = width;
    UWORD y, *p;

    D(bug("[AmigaVideo] %s()\n", __func__));

    OOP_GetAttr(msg->shape, aHidd_BitMap_PixFmt, (IPTR*)&bmPFObj);
    OOP_GetAttr(bmPFObj, aHidd_PixFmt_ColorModel, &bmcmod);
    if (bmcmod == vHidd_ColorModel_TrueColor)
    {
        OOP_GetAttr(bmPFObj, aHidd_PixFmt_StdPixFmt, (IPTR*)&pf);
        bmPF = (HIDDT_PixelFormat *)HIDD_Gfx_GetPixFmt(o, pf);
    }

    if (csd->aga && csd->aga_enabled && width > 16)
        csd->fmode_spr = 2;
    else
        csd->fmode_spr = 0;
    D(bug("[AmigaVideo] %s: fmode_spr = %x\n", __func__, csd->fmode_spr));
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
                        c = HIDD_BM_GetPixel(msg->shape, xx + x, y);
                    else
                    {
                        HIDDT_Pixel pix = HIDD_BM_GetPixel(msg->shape, xx + x, y);
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
    setspritepos(csd, csd->spritex, csd->spritey, data->res, data->interlace);
    setspritevisible(csd, csd->cursorvisible);
    return TRUE;
}

void setspritepos(struct amigavideo_staticdata *csd, WORD x, WORD y, UBYTE res, BOOL interlace)
{
    UWORD ctl, pos;

    csd->spritex = x;
    csd->spritey = y;
    if (!csd->sprite || csd->sprite_height == 0)
        return;

    x += csd->sprite_offset_x << res;
    x <<= (2 - res); // convert x to shres coordinates
    x += (csd->startx - 1) << 2; // display left edge offset
 
    if (interlace)
        y >>= 1; // y is always in nonlaced
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
    D(bug("[AmigaVideo] %s()\n", __func__));

    csd->cursorvisible = visible;
    if (visible) {
        if (csd->copper1_spritept) {
            UWORD *p = csd->sprite;
            struct amigabm_data *bm;
            ForeachNode(csd->compositedbms, bm)
            {
                if (((bm->interlace) && ((csd->spritey >> 1) < (bm->topedge + bm->displayheight))) ||
                    ((!bm->interlace) && (csd->spritey < (bm->topedge + bm->displayheight))))
                {
                    setfmode(csd, bm);
                    break;
                }
            }
            csd->copper1_spritept[0] = (UWORD)(((ULONG)p) >> 16);
            csd->copper1_spritept[2] = (UWORD)(((ULONG)p) >> 0);
        }
    } else {
        setnullsprite(csd);
    }
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

    if ((bm->bmcl) && (bm->bmcl->CopLStart))
        setcoppercolors(csd, bm, bm->palette);

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

static void gfx_stripbm(struct amigavideo_staticdata *csd, struct amigabm_data *bm)
{
    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;
    struct copper2data *c2dp, *c2ds, *c2dip, *c2dis;
    struct amigabm_data *bmtmp;

    D(bug("[AmigaVideo] %s(0x%p)\n", __func__, bm));

    c2dp = (struct copper2data *)bm->copld.cnode.mln_Pred;
    c2ds = (struct copper2data *)bm->copld.cnode.mln_Succ;
    Remove(&bm->copld.cnode);
    D(bug("[AmigaVideo] %s:   removed bm->copld node\n", __func__);)

    if (csd->interlaced || bm->interlace)
    {
        c2dip = (struct copper2data *)bm->copsd.cnode.mln_Pred;
        c2dis = (struct copper2data *)bm->copsd.cnode.mln_Succ;
        Remove(&bm->copsd.cnode);
        D(bug("[AmigaVideo] %s:   removed bm->copsd node\n", __func__);)
    }

    if (c2dp && c2dp->cnode.mln_Pred)
    {
        bmtmp = BMDATFROMCOPLD(c2ds);
        setcopperlisttail(csd, c2dp->copper2_tail, bmtmp->bmcl->CopLStart, TRUE);
        D(bug("[AmigaVideo] %s: CopL @ 0x%p point to CopL @ 0x%p\n", __func__, c2dp, c2ds);)
        if (csd->interlaced || bm->interlace)
        {
            bmtmp = BMDATFROMCOPSD(c2dis);
            setcopperlisttail(csd, c2dip->copper2_tail, bmtmp->bmcl->CopSStart, TRUE);
            D(bug("[AmigaVideo] %s: CopS @ 0x%p point to CopS @ 0x%p\n", __func__, c2dip, c2dis);)
        }
    }
    else
    {
        struct copper2data *c2t;
        UWORD *copstart;

        D(bug("[AmigaVideo] %s: screen-bitmap was first on copperlist chain\n", __func__);)
        bmtmp = BMDATFROMCOPLD(c2ds);

        /* first, replace any tail refrence ... */
        if (!IsListEmpty(&csd->c2ifragments))
        {
            c2t = (struct copper2data *)GetTail(&csd->c2ifragments);
            setcopperlisttail(csd, c2t->copper2_tail, bmtmp->bmcl->CopLStart, FALSE);
            D(bug("[AmigaVideo] %s: CopS @ 0x%p point to CopL @ 0x%p\n", __func__, c2t, c2ds);)
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
            D(bug("[AmigaVideo] %s: CopL @ 0x%p point to CopL @ 0x%p\n", __func__, c2t, c2ds);)
        }

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


}

static AROS_INTH1(gfx_vblank, struct amigavideo_staticdata*, csd)
{ 
    AROS_INTFUNC_INIT

    struct GfxBase *GfxBase = (APTR)csd->cs_GfxBase;
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    BOOL lof = (custom->vposr & 0x8000) != 0;

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

    if (csd->updatescroll) {
        struct amigabm_data *bm, *bmsafe, *bmtmp;
        ForeachNodeSafe(csd->compositedbms, bm, bmsafe)
        {
            UWORD bmend;
            if (bm->node.ln_Succ && bm->node.ln_Succ->ln_Succ && (bm->node.ln_Succ == (struct Node *)csd->updatescroll))
            {
                /* adjust the screen situated above the moved screen-bitmap */
                bmend = csd->updatescroll->topedge - (csd->updatescroll->copld.extralines + 1);
                if ((bm->interlace) && (!csd->updatescroll->interlace))
                    bmend <<= 1;
                else if ((!bm->interlace) && (csd->updatescroll->interlace))
                    bmend >>= 1;

                /* only adjust if enough is visible - otherwise it will be obscured, and hidden in the next case... */
                if ((bm->displayheight = limitheight(csd, (bmend - bm->topedge), bm->interlace, FALSE)) > 1)
                    setcopperscroll(csd, bm, csd->interlaced | bm->interlace);
            }
            else if (bm == csd->updatescroll)
            {
                UWORD toptmp; 

                if (bm->updtop < bm->topedge)
                {
                    struct Node *tmpnode;
                    UWORD topvis = bm->topedge; 

                    /* check if any of the obscured screens have become visible .. */
                    ForeachNodeSafe(csd->obscuredbms, bmtmp, tmpnode)
                    {
                        if (bmtmp->node.ln_Pri > bm->node.ln_Pri)
                        {
                            toptmp = bmtmp->topedge;
                            if ((bm->interlace) && (!bmtmp->interlace))
                                toptmp <<= 1;
                            else if ((!bm->interlace) && (bmtmp->interlace))
                                toptmp >>= 1;

                            if (toptmp < topvis)
                            {
                                D(bug("[AmigaVideo] %s: screen-bitmap with bmdata @ 0x%p and depth %d became visible\n", __func__, bmtmp, bmtmp->node.ln_Pri);)

                                topvis = toptmp;
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

                            gfx_stripbm(csd, bmtmp);

                            D(bug("[AmigaVideo] %s: screen-bitmap with bmdata @ 0x%p and depth %d is now obscured\n", __func__, bmtmp, bmtmp->node.ln_Pri);)

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

                    bm->displayheight = limitheight(csd, (bmend - bm->topedge), bm->interlace, FALSE);                    
                }
                else
                    bm->displayheight = limitheight(csd, (bm->height - bm->topedge), bm->interlace, FALSE);
                setcopperscroll(csd, csd->updatescroll, csd->interlaced | csd->updatescroll->interlace);
                bm->updtop = bm->topedge;
                bm->updleft = bm->leftedge;

            }
        }
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

#if (1)
    /* TODO: This shouldnt be done in the gfx driver!
     * move to somewhere more appropriate */

    /* Reset audio registers to values that help emulation
     * if some program enables audio DMA without setting period
     * or length. Very high period emulation is very CPU intensive.
     */
    for (i = 0; i < 4; i++) {
        custom->aud[i].ac_vol = 0;
        custom->aud[i].ac_per = 100;
        custom->aud[i].ac_len = 1000;
    }
#endif

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

    vposr = custom->vposr;
    csd->aga = (vposr & 0x0f00) == 0x0300;
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
    csd->max_colors = csd->aga ? 256 : 32;

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

    D(bug("[AmigaVideo] %s: Copperlist0 %p\n", __func__, csd->copper1));
}

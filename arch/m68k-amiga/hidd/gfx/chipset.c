
#include <proto/exec.h>

#include <exec/libraries.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hidd/graphics.h>

#include "amigavideogfx.h"
#include "amigavideobitmap.h"
#include "chipset.h"

#define DEBUG 1
#include <aros/debug.h>

static const UBYTE fetchunits[] = { 3,3,3,0, 4,3,3,0, 5,4,3,0 };
static const UBYTE fetchstarts[] = { 3,2,1,0, 4,3,2,0, 5,4,3,0 };
static const UBYTE fm_maxplanes[] = { 3,2,1,0, 3,3,2,0, 3,3,3,0 };

void resetcustom(void)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    custom->fmode = 0;
    custom->bplcon0 = 0x0200;
    custom->bplcon1 = 0x0000;
    custom->bplcon2 = 0x0000;
    custom->bplcon3 = 0x0c02;
    custom->bplcon4 = 0x0011;
    custom->vposw = 0x8000;
    custom->color[0] = 0x0444;

    /* workaround for recent WinUAE initial blit bug */
    custom->bltapt = 0;
    custom->bltcon0 = 0x0800;
    custom->bltcon1 = 0x0000;
    custom->bltsize = (1 << 6) | 1;
    
}

static AROS_UFH4(ULONG, gfx_vblank,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, datap, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{ 
    AROS_USERFUNC_INIT

    struct amigavideo_staticdata *data = (struct amigavideo_staticdata*)datap;
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    BOOL start = FALSE;

    data->framecounter++;
    if (data->sprite) {
    	UWORD *p = data->sprite;
    	p[0] = data->spritepos;
    	p[1 << data->fmode_spr] = data->spritectl;
    }
    if (data->mode == 1) {
    	if (data->interlace) {
	    custom->bplcon0 = 0x0204;
	    if (custom->vposr & 0x8000)
	    	start = TRUE;
	} else {
	    start = TRUE;
	}
    	if (start) {
            custom->cop2lc = (ULONG)data->copper2.copper2;
  	    custom->copjmp1 = 0x0000;
    	    custom->dmacon = 0x8100;
    	    data->mode = 0;
    	}
    }

    return 0;
	
    AROS_USERFUNC_EXIT
}

static void waitvblank(struct amigavideo_staticdata *data)
{
    // ugly busy loop for now..
    UWORD fc = data->framecounter;
    while (fc == data->framecounter);
}

void initcustom(struct amigavideo_staticdata *data)
{
    UBYTE i;
    UWORD *c;
    UWORD vposr;
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    resetcustom();

    data->gfxbase = (struct GfxBase*)OpenLibrary("graphics.library", 0);

    data->inter.is_Code         = (APTR)gfx_vblank;
    data->inter.is_Data         = data;
    data->inter.is_Node.ln_Name = "GFX VBlank server";
    data->inter.is_Node.ln_Pri  = 25;
    data->inter.is_Node.ln_Type = NT_INTERRUPT;
    AddIntServer(INTB_VERTB, &data->inter);

    data->startx = 0x80;
    data->starty = 0x28;

    vposr = custom->vposr & 0x7f00;
    data->aga = vposr >= 0x2200;
    data->ecs_agnus = vposr >= 0x2000;
    data->max_colors = data->aga ? 256 : 32;
    data->palette = AllocVec(data->max_colors * 3, MEMF_CLEAR);
    data->copper1 = AllocVec(20 * 2 * sizeof(WORD), MEMF_CLEAR | MEMF_CHIP);
    data->sprite_null = AllocMem(2 * 8, MEMF_CLEAR | MEMF_CHIP);
    c = data->copper1;
    for (i = 0; i < 8; i++) {
	*c++ = 0x0120 + i * 4;
	if (i == 0)
	    data->copper1_spritept = c;
	*c++ = (UWORD)(((ULONG)data->sprite_null) >> 16);
	*c++ = 0x0122 + i * 4;
	*c++ = (UWORD)(((ULONG)data->sprite_null) >> 0);
    }
    *c++ = 0x008a;
    *c++ = 0x0000;
    data->copper2_backup = c;
    *c++ = 0xffff;
    *c++ = 0xfffe;
    custom->cop1lc = (ULONG)data->copper1;
    custom->cop2lc = (ULONG)data->copper2_backup;
    custom->dmacon = 0x8000 | 0x0080 | 0x0020;
    data->bplcon3 = ((data->res + 1) << 6) | 2; // spriteres + bordersprite
    
    data->gfxbase->copinit = (struct copinit*)data->copper1;

    D(bug("Copperlist0 %p\n", data->copper1));

}

static void setfmode(struct amigavideo_staticdata *data)
{
    UWORD fmode;
    fmode  =  data->fmode_bpl == 2 ? 3 : data->fmode_bpl;
    fmode |= (data->fmode_spr == 2 ? 3 : data->fmode_spr) << 2;
    *data->copper2.copper2_fmode = fmode;
    if (data->interlace)
    	*data->copper2i.copper2_fmode = fmode;
}

static void setcoppercolors(struct amigavideo_staticdata *data)
{
    UWORD i;
 
    if (!data->copper2.copper2_palette)
	return;
    if (data->aga) {
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
    } else {
    	for (i = 0; i < data->use_colors; i++) {
 	    UWORD val = ((data->palette[i * 3 + 0] >> 4) << 8) | ((data->palette[i * 3 + 1] >> 4) << 4) | ((data->palette[i * 3 + 2] >> 4) << 0);
 	    data->copper2.copper2_palette[i * 2 + 1] = val;
 	    if (data->interlace)
 	    	data->copper2i.copper2_palette[i * 2 + 1] = val;
	}
    }
}

void resetmode(struct amigavideo_staticdata *data)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    D(bug("resetmode\n"));

    data->disp = NULL;

    custom->dmacon = 0x0100;

    custom->cop2lc = (ULONG)data->copper2_backup;

    waitvblank(data);

    FreeVec(data->copper2.copper2);
    data->copper2.copper2 = NULL;
    FreeVec(data->copper2i.copper2);
    data->copper2i.copper2 = NULL;

    resetcustom();

    data->depth = 0;
}

static void setcopperscroll2(struct amigavideo_staticdata *data, struct amigabm_data *bm, struct copper2data *c2d)
{
    UWORD *copptr = c2d->copper2_scroll, *copbpl;
    WORD scroll, yscroll;
    WORD y = data->starty + (bm->topedge >> data->interlace);
    WORD x = data->startx + bm->leftedge;
    WORD yend, i;
    
    yscroll = 0;
    if (y < 10) {
    	yscroll = y - 10;
    	y = 10;
    }
    if (x < 0)
    	x = 0;
    copptr[1] = 0x0a81; //(y << 8) + (x + 1);
    copptr[3] = 0x40c1; //((y + (bm->rows >> data->interlace)) << 8) + ((x + 1 + (bm->width >> data->res)) & 0x00ff);
    copptr[5] = data->ddfstrt;
    copptr[7] = data->ddfstop;

    copbpl = c2d->copper2_bpl;
    for (i = 0; i < bm->depth; i++) {
	ULONG pptr = (ULONG)(bm->planes[i]);
	if (data->interlace)
	    pptr += bm->bytesperrow;
	pptr -= (bm->leftedge >> (8 << data->fmode_bpl)) & ~((2 << data->fmode_bpl) - 1);
	pptr -= (yscroll * bm->bytesperrow) << (data->interlace ? 1 : 0);
	copbpl[1] = (UWORD)(pptr >> 16);
	copbpl[3] = (UWORD)(pptr >> 0);
	copbpl += 4;
    }

    scroll = bm->leftedge & ((16 << data->fmode_bpl) - 1);
    copptr[9] = (scroll & 0x0f) | ((scroll & 0x0f) << 4) | ((scroll >> 4) << 10) | ((scroll >> 4) << 14);

    yend = y + bm->height + yscroll;
    if (yend > 312)
    	yend = 312;
    copptr = c2d->copper2_bplcon0;
    copptr[4] = (yend << 8) | 0x05;
    if (yend < 256) {
        copptr[2] = 0x00df;
        copptr[3] = 0x00fe;
    } else {
    	copptr[2] = 0xffdf;
    	copptr[3] = 0xfffe;
    }

    copptr = c2d->copper2;
    copptr[0] = ((y - data->extralines) << 8) | 0x05;
    copptr = c2d->copper2_bplcon0;
    copptr[-2] = (y << 8) | 0x05;
    
}

static void setcopperscroll(struct amigavideo_staticdata *data, struct amigabm_data *bm)
{
    setcopperscroll2(data, bm, &data->copper2);
    if (data->interlace)
    	setcopperscroll2(data, bm, &data->copper2i);
}

static UWORD get_copper_list_length(UBYTE aga, UBYTE depth)
{
    UWORD v;

    if (aga) {
    	v = 1000 + ((1 << depth) + 1 + (1 << depth) / 32 + 1) * 2;
    } else {
    	v = 1000;
    }
    return v * 2;
}

static void createcopperlist(struct amigavideo_staticdata *data, struct amigabm_data *bm, struct copper2data *c2d, BOOL lace)
{
    UWORD *c;
    UWORD i;
    UWORD bplcon0, bplcon0_null;
    ULONG pptr;

    c = c2d->copper2;
    D(bug("Copperlist%d %p\n", lace ? 2 : 1, c));

    bplcon0_null = bplcon0 = 0x0201 | (data->interlace ? 4 : 0);

    *c++ = 0xffff;
    *c++ = 0xfffe;

    *c++ = 0x0100;
    *c++ = bplcon0_null;

    c2d->copper2_bpl = c;
    for (i = 0; i < bm->depth; i++) {
	pptr = (ULONG)(bm->planes[i]);
	if (lace)
	    pptr += bm->bytesperrow;
	*c++ = 0xe0 + i * 4;
	*c++ = (UWORD)(pptr >> 16);
	*c++ = 0xe2 + i * 4;
	*c++ = (UWORD)(pptr >> 0);
    }

    data->use_colors = 1 << bm->depth;
    // need to update sprite colors
    if (data->use_colors < 20)
    	data->use_colors = 20;

    c2d->copper2_scroll = c;
    *c++ = 0x008e;
    *c++ = 0;
    *c++ = 0x0090;
    *c++ = 0;
    *c++ = 0x0092;
    *c++ = 0;
    *c++ = 0x0094;
    *c++ = 0;
    *c++ = 0x0102;
    *c++ = 0;
    *c++ = 0x0108;
    *c++ = 0x0000 + (data->interlace ? bm->bytesperrow : 0) + data->modulo;
    *c++ = 0x010a;
    *c++ = 0x0000 + (data->interlace ? bm->bytesperrow : 0) + data->modulo;
    *c++ = 0x0104;
    *c++ = 0x0024;

    c2d->copper2_fmode = NULL;
    if (data->aga) {
    	*c++ = 0x01fc;
    	c2d->copper2_fmode = c;
    	*c++ = 0;
    }

    if (data->res == 1)
	bplcon0 |= 0x8000;
    else if (data->res == 2)
	bplcon0 |= 0x0040;
    if (bm->depth > 7)
	bplcon0 |= 0x0010;
    else
	bplcon0 |= bm->depth << 12;
    if (data->interlace)
	bplcon0 |= 0x0004;

    c2d->copper2_palette = c;
    if (data->aga) {
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
    *c++ = bplcon0_null;

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

BOOL setmode(struct amigavideo_staticdata *data, struct amigabm_data *bm)
{
    UWORD ddfstrt, ddfstop;
    UBYTE fetchunit, fetchstart, maxplanes;
    
    resetmode(data);

    data->fmode_bpl = data->aga ? 2 : 0;

    fetchunit = fetchunits[data->fmode_bpl * 4 + data->res];
    fetchstart = fetchstarts[data->fmode_bpl * 4 + data->res];
    maxplanes = fm_maxplanes[data->fmode_bpl * 4 + data->res];

    D(bug("setmode bm=%x w=%d h=%d d=%d fu=%d fs=%d\n",
    	bm, bm->width, bm->height, bm->depth, fetchunit, fetchstart));
    
    ddfstrt = (data->startx / 2) & ~((1 << fetchunit) - 1);
    ddfstop = ddfstrt + ((bm->width / 4 + ((1 << fetchunit) - 1) - 2 * (1 << fetchunit)) & ~((1 << fetchunit) - 1));
    if (ddfstop >= 0xd4)
	ddfstop = 0xd4;
    data->modulo = (ddfstop + 2 * (1 << fetchunit) - ((1 << maxplanes) - 1) - ddfstrt) * 4;
    data->modulo = bm->width - data->modulo;
    data->modulo /= 8;
    data->modulo &= ~((2 << data->fmode_bpl) - 1);
    ddfstrt -= (1 << maxplanes);
    data->ddfstrt = ddfstrt;
    data->ddfstop = ddfstop;

    data->copper2.copper2 = AllocVec(get_copper_list_length(data->aga, bm->depth), MEMF_CLEAR | MEMF_CHIP);
    if (data->interlace)
    	data->copper2i.copper2 = AllocVec(get_copper_list_length(data->aga, bm->depth), MEMF_CLEAR | MEMF_CHIP);
    createcopperlist(data, bm, &data->copper2, 0);
    if (data->interlace)
    	createcopperlist(data, bm, &data->copper2i, 1);
 
    setfmode(data);
    setcopperscroll(data, bm);
    data->depth = bm->depth;

    data->mode = 1;
    while (data->mode);

    setcoppercolors(data);
    setspritepos(data, data->spritex, data->spritey);

    data->disp = bm;
    return 1;
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

BOOL setsprite(struct amigavideo_staticdata *data, WORD width, WORD height, struct pHidd_Gfx_SetCursorShape *shape)
{
    UWORD fetchsize;
    UWORD bitmapwidth = width;
    UWORD y, *p;
    
    if (data->aga && width > 16) {
    	fetchsize = 8;
    	width = 64;
    	data->fmode_spr = 2;
    } else {
    	fetchsize = 2;
    	width = 16;
    	data->fmode_spr = 0;
    }

    if (width != data->sprite_width || height != data->sprite_height) {
    	resetsprite(data);
    	data->spritedatasize = fetchsize * 2 + fetchsize * height * 2 + fetchsize * 2;
    	data->sprite = AllocMem(data->spritedatasize, MEMF_CHIP | MEMF_CLEAR);
    	if (!data->sprite)
	    return FALSE;
	data->sprite_width = width;
	data->sprite_height = height;
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
    x += (data->startx * 2);
    x <<= (2 - data->res); // convert x to shres coordinates
    if (data->interlace)
    	y /= 2; // y is always in nonlaced
    y += data->starty;
    pos = (y << 8) | (x >> 3);
    ctl = ((y + data->sprite_height) << 8);
    ctl |= ((y >> 8) << 2) | (((y + data->sprite_height) >> 8) << 1) | ((x >> 2) & 1) | ((x & 3) << 3);
    data->spritepos = pos;
    data->spritectl = ctl;
}

void setspritevisible(struct amigavideo_staticdata *data, BOOL visible)
{
    if (data->cursorvisible == visible)
    	return;
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
    setcopperscroll(data, bm);
}

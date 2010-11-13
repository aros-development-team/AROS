
#include <proto/exec.h>

#include <hardware/custom.h>
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
    custom->bplcon3 = 0x0c00;
    custom->bplcon4 = 0x0011;
    custom->vposw = 0x8000;
}

static void waitvblank(void)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    while((custom->vposr & 0x000f) == 0);
    while((custom->vposr & 0x000f) != 0);
}

static UBYTE cursorcolors[] = { 0xdd,0x22,0x22,  0x00,0x00,0x00, 0xff,0xcc,0xaa };
void initcustom(struct amigavideo_staticdata *data)
{
    UBYTE i, j;
    UWORD *c;
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    resetcustom();


    data->startx = 0x80;
    data->starty = 0x28;

    data->aga = (custom->vposr & 0x7f00) >= 0x2200;
    data->max_colors = data->aga ? 256 : 32;
    data->palette = AllocVec(data->max_colors * 3, MEMF_CLEAR);
    data->copper1 = AllocVec(100 * 2, MEMF_CLEAR | MEMF_CHIP);
    data->sprite_null = AllocVec(2 * 8, MEMF_CLEAR | MEMF_CHIP);
    c = data->copper1;
    *c++ = 0x0180;
    *c++ = 0x0444;
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
    data->bplcon3 = (data->res + 1) << 6; // spriteres
    for (i = 17, j = 0; i <= 19; i++, j++) {
    	data->palette[i * 3 + 0] = cursorcolors[j * 3 + 0];
    	data->palette[i * 3 + 1] = cursorcolors[j * 3 + 1];
    	data->palette[i * 3 + 2] = cursorcolors[j * 3 + 2];
    }
}    

static void setcoppercolors(struct amigavideo_staticdata *data)
{
    UWORD i;
 
    if (!data->copper2_palette)
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
   	    data->copper2_palette[i * 2 + off] = valhi;
   	    data->copper2_palette_aga_lo[i * 2 + off] = vallo;
   	    if (data->interlace) {
    	    	data->copper2i_palette[i * 2 + off] = valhi;
   	    	data->copper2i_palette_aga_lo[i * 2 + off] = vallo;
   	    }	
   	}   
    } else {
    	for (i = 0; i < data->use_colors; i++) {
 	    UWORD val = ((data->palette[i * 3 + 0] >> 4) << 8) | ((data->palette[i * 3 + 1] >> 4) << 4) | ((data->palette[i * 3 + 2] >> 4) << 0);
 	    data->copper2_palette[i * 2 + 1] = val;
 	    if (data->interlace)
 	    	data->copper2i_palette[i * 2 + 1] = val;
	}
    }
}

void resetmode(struct amigavideo_staticdata *data)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    bug("resetmode\n");

    custom->dmacon = 0x0100;

    custom->cop2lc = (ULONG)data->copper2_backup;

    waitvblank();

    FreeVec(data->copper2);
    data->copper2 = NULL;
    FreeVec(data->copper2i);
    data->copper2i = NULL;

    resetcustom();

    data->depth = 0;
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

BOOL setmode(struct amigavideo_staticdata *data, struct planarbm_data *bm)
{
    UWORD i;
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    UWORD *c, *ci, *ctemp;
    ULONG pptr;
    UWORD bplcon0;
    UWORD fmode = 0;
    UWORD ddfstrt, ddfstop;
    UBYTE fetchunit, fetchstart;
    
    resetmode(data);

    data->res = 1;
    data->interlace = 1;

    fmode = data->aga ? 2 : 0;

    bug("setmode bm=%x w=%d h=%d d=%d\n", bm, bm->width, bm->rows, bm->depth);
    
    fetchunit = fetchunits[fmode * 4 + data->res];
    fetchstart = fetchstarts[fmode * 4 + data->res];
    ddfstrt = (data->startx / 2) & ~((1 << fetchunit) - 1);
    ddfstop = ddfstrt + ((bm->width / 4 + ((1 << fetchunit) - 1) - 2 * (1 << fetchunit)) & ~((1 << fetchunit) - 1));
    ddfstrt -= (1 << fetchstart);
    if (ddfstop >= 0xd4)
	ddfstop = 0xd4;

    c = data->copper2 = AllocVec(get_copper_list_length(data->aga, bm->depth), MEMF_CLEAR | MEMF_CHIP);
    ci = NULL;
    if (data->interlace) {
	ci = data->copper2i = AllocVec(get_copper_list_length(data->aga, bm->depth), MEMF_CLEAR | MEMF_CHIP);
    	for (i = 0; i < bm->depth; i++) {
	    pptr = (ULONG)(bm->planes[i]) + bm->bytesperrow;
	    *ci++ = 0xe0 + i * 4;
	    *ci++ = (UWORD)(pptr >> 16);
	    *ci++ = 0xe2 + i * 4;
	    *ci++ = (UWORD)(pptr >> 0);
	
	}
    }

    for (i = 0; i < bm->depth; i++) {
	pptr = (ULONG)(bm->planes[i]);
	*c++ = 0xe0 + i * 4;
	*c++ = (UWORD)(pptr >> 16);
	*c++ = 0xe2 + i * 4;
	*c++ = (UWORD)(pptr >> 0);
    }

    ctemp = c;
    data->copper2_palette = c;
    data->copper2i_palette = ci;
    data->use_colors = 1 << bm->depth;
    // need to update sprite colors
    if (data->use_colors < 20)
    	data->use_colors = 20;
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
	data->copper2_palette_aga_lo = c;
	data->copper2i_palette_aga_lo = ci + (data->copper2_palette_aga_lo - data->copper2_palette);
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

   if (data->aga) {
    	*c++ = 0x01fc;
    	*c++ = fmode == 2 ? 3 : fmode;
    	*c++ = 0x0106;
    	*c++ = data->bplcon3;
    }
    *c++ = 0x008e;
    *c++ = (data->starty << 8) + (data->startx + 1);
    *c++ = 0x0090;
    *c++ = ((data->starty + (bm->rows >> data->interlace)) << 8) + ((data->startx + 1 + (bm->width >> data->res)) & 0x00ff);
    *c++ = 0x0092;
    *c++ = ddfstrt;
    *c++ = 0x0094;
    *c++ = ddfstop;
    *c++ = 0x0108;
    *c++ = 0x0000 + (data->interlace ? bm->bytesperrow : 0);
    *c++ = 0x010a;
    *c++ = 0x0000 + (data->interlace ? bm->bytesperrow : 0);
    *c++ = 0x0104;
    *c++ = 0x0024;
    *c++ = 0x0100;

    bplcon0 = 0x0201;
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
    *c++ = bplcon0;

    if (data->interlace) {
	memcpy (ci, ctemp, (c - ctemp) * 2);
	ci += c - ctemp;
	pptr = (ULONG)(data->copper2);
	*ci++ = 0x0084;
	*ci++ = (UWORD)(pptr >> 16);
	*ci++ = 0x0086;
	*ci++ = (UWORD)(pptr >> 0);
	*ci++ = 0xffff;
	*ci++ = 0xfffe;

	pptr = (ULONG)(data->copper2i);
	*c++ = 0x0084;
	*c++ = (UWORD)(pptr >> 16);
	*c++ = 0x0086;
	*c++ = (UWORD)(pptr >> 0);
    }
    *c++ = 0xffff;
    *c++ = 0xfffe;

    custom->cop2lc = (ULONG)data->copper2;

    if (data->interlace) {
    	for (;;) {
    	    custom->bplcon0 = 0x0204;
    	    while ((custom->vposr & 0x8000));
   	    while (!(custom->vposr & 0x8000));
    	    custom->copjmp1 = 0;
    	    if (!(custom->vposr & 0x8000))
    	    	break;
    	}
    }

    custom->dmacon = 0x8100;
    
    data->depth = bm->depth;

    setcoppercolors(data);
    return 1;
}

static void setnullsprite(struct amigavideo_staticdata *data)
{
    if (data->copper1_spritept) {
	data->copper1_spritept[0] = (UWORD)(((ULONG)data->sprite_null) >> 16);
	data->copper1_spritept[2] = (UWORD)(((ULONG)data->sprite_null) >> 0);
    }
 }
 
void resetsprite(struct amigavideo_staticdata *data)
{
    setnullsprite(data);
    FreeVec(data->sprite);
    data->sprite = NULL;
    data->sprite_width = data->sprite_height = 0;
}

BOOL setsprite(struct amigavideo_staticdata *data, WORD width, WORD height, struct pHidd_Gfx_SetCursorShape *shape)
{
    UWORD x, y, *p;

    if (width != data->sprite_width || height != data->sprite_height) {
    	resetsprite(data);
    	data->sprite = AllocVec(2 + 2 + (width + 15) / 8 * height * 2 + (2 + 2) * 2, MEMF_CHIP | MEMF_CLEAR);
    	if (!data->sprite)
	    return FALSE;
	memset(data->sprite + 2, 0xaa, width / 8 * height * 2);
	data->sprite_width = width;
	data->sprite_height = height;
    }
    p = data->sprite + 2;
    for(y = 0; y < height; y++) {
    	UWORD pix1 = 0, pix2 = 0;
    	for(x = 0; x < width; x++) {
    	    UBYTE c = HIDD_BM_GetPixel(shape->shape, x, y);
    	    pix1 <<= 1;
    	    pix2 <<= 1;
    	    pix1 |= (c & 1) ? 1 : 0;
    	    pix2 |= (c & 2) ? 1 : 0;
    	}
    	*p++ = pix1;
    	*p++ = pix2;
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
    ctl = (y << 8) | (x >> 3);
    pos = ((y + data->sprite_height) << 8);
    pos |= ((y >> 8) << 2) | (((y + data->sprite_height) >> 8) << 1) | ((x >> 2) & 1) | ((x & 3) << 3);
    data->sprite[0] = ctl;
    data->sprite[1] = pos;
}

void setspritevisible(struct amigavideo_staticdata *data, BOOL visible)
{
    if (data->cursorvisible == visible)
    	return;
    data->cursorvisible = visible;
    if (visible) {
    	if (data->copper1_spritept) {
	    data->copper1_spritept[0] = (UWORD)(((ULONG)data->sprite) >> 16);
	    data->copper1_spritept[2] = (UWORD)(((ULONG)data->sprite) >> 0);
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
    }
    if (visible)
	setcoppercolors(data);
    return TRUE;
}

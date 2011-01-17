/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.

    Desc: Detect custom chipset revisions and PAL/NTSC state
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>

static UWORD getline(void)
{
	volatile struct Custom *custom = (struct Custom*)0xdff000;
	UBYTE vposb;
	UWORD vpos;

	vposb = custom->vhposr >> 8;
	while (vposb == (custom->vhposr >> 8));
	vpos = ((custom->vposr & 7) << 8) | (custom->vhposr >> 8);
	return vpos;
}

static UWORD gethighestline(UWORD linecnt)
{
	volatile struct Custom *custom = (struct Custom*)0xdff000;
	UBYTE vposb;
	UWORD vpos, highest;
	
	highest = 0;
	while (linecnt-- != 0) {
		vposb = custom->vhposr >> 8;
		while (vposb == (custom->vhposr >> 8));
		vposb++;
		vpos = ((custom->vposr & 7) << 8) | vposb;
		if (vpos > highest)
			highest = vpos;
	}
	return highest;
}	

void InitCustom(struct GfxBase *gfx)
{
	volatile struct Custom *custom = (struct Custom*)0xdff000;
	UWORD vposr, deniseid;
	UWORD flags = 0;
	UBYTE chipflags = 0;
	UWORD pos, i;
	
	custom->vposw = 0x8000;
	custom->bplcon0 = 0x0200;
	Disable();
	while(getline() != 0);
	pos = gethighestline(400);
	Enable();
	if (pos > 300)
		flags |= PAL;
	else
		flags |= NTSC;
	
   	vposr = custom->vposr & 0x7f00;
   	if (vposr >= 0x2000)
   		chipflags |= GFXF_HR_AGNUS;
   	deniseid = custom->deniseid & 0x00ff;
   	custom->fmode = 0;
   	if ((custom->deniseid & 0x00ff) == deniseid) {
   		if (deniseid == 0xfc)
   			chipflags |= GFXF_HR_DENISE;
   		if (deniseid == 0xf8)
   			chipflags |= GFXF_AA_LISA;
   	}
   	if (vposr >= 0x2200)
   		chipflags |= GFXF_AA_ALICE;
 
	gfx->DisplayFlags = flags;
	gfx->ChipRevBits0 = chipflags;
}

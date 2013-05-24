/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.

    Desc: Detect custom chipset revisions and PAL/NTSC state
    Lang: english
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>
#include <hardware/cia.h>

#include <timer_platform.h>

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
	UWORD vposr;
	UWORD flags = 0;
	UWORD pos;

	Disable();

	custom->vposw = 0x8000;
	custom->bplcon0 = 0x0200;

	while(getline() != 0);
	pos = gethighestline(400);
	if (pos > 300)
		flags |= PAL;
	else
		flags |= NTSC;
	
   	vposr = custom->vposr & 0x7f00;
   	if (!(vposr & 0x1000))
   		flags |= REALLY_PAL;
   	custom->fmode = 0;

   	Enable();

   	if (vposr >= 0x2200) {
		gfx->MemType = BUS_32 | DBL_CAS;
	}

	gfx->DisplayFlags = flags;

	SysBase->VBlankFrequency = (flags & PAL) ? 50 : 60;
}

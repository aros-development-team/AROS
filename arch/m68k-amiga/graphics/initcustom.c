/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.

    Desc: Detect custom chipset revisions and PAL/NTSC state
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>
#include <hardware/cia.h>

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
	volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
	UWORD vposr, deniseid;
	UWORD flags = 0;
	UBYTE chipflags = 0, todlow;
	UWORD pos, todlo, pshz, todcnt;

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
 
	/* check powersupply tick rate */
	ciaa->ciacra = 0x00;
	ciaa->ciatodhi = 0;
	ciaa->ciatodmid = 0;
	ciaa->ciatodlow = 0;
	ciaa->ciatalo = 0xff;
	ciaa->ciatahi = 0xff;
	todlo = ciaa->ciatodlow;
	while (todlo == ciaa->ciatodlow);
	ciaa->ciacra = 0x01;
	todlo = ciaa->ciatodlow;
	while (todlo == ciaa->ciatodlow);
	ciaa->ciacra = 0x00;
	todcnt = ~(((ciaa->ciatahi << 8) | ciaa->ciatalo) + 1);

    	/* 50Hz/60Hz ticks:
    	 * 50Hz PAL  14188
    	 * 60Hz NTSC 11932
    	 * 50Hz NTSC 14318
    	 * 60Hz PAL  11823
    	 */
    	if (todcnt > 14188 + (14318 - 14188) / 2) {
    		pshz = 50;
    	} else if (todcnt <= 11823 + (11932 - 11823) / 2) {
    		pshz = 60;
    	} else if (todcnt > 14188 - (14188 - 11932) / 2) {
    		pshz = 50;
    	} else {
     		pshz = 60;
	}

	Enable();

	gfx->DisplayFlags = flags;
	gfx->ChipRevBits0 = chipflags;
	SysBase->PowerSupplyFrequency = pshz;
	SysBase->VBlankFrequency = (flags & PAL) ? 50 : 60;

}

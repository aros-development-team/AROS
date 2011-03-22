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

/*
 * TODO: it's not architecturally good to have hardware-specific code in graphics.library.
 * During init graphics.library instantiates default graphics driver. One of ideas is to
 * instantiate Amiga(tm) chipset driver instead on appropriate hardware. In this case the
 * driver will be able to do this stuff for us.
 * At the other hand, what if the user owns an Amiga with VGA monitor and graphics card and
 * intentionally doesn't want to use chipset driver at all (assuming loadable modular kickstart)?
 * This stuff is vital for correct functioning of timer.device, so it's logical to move this
 * code to kernel.resource instead. But in this case we would have to: a) duplicate the code
 * to set DisplayFlags and ChipsetFlags; or b) Add these flags to the list of kernel.resource
 * attributes (which is again not quite good because these things are very Amiga-specific).
 * UPD: c) We could move this code to something like amigacustom.resource which would be
 * totally hardware-specific part. Presence of this resource would clearly indicate to the 
 * software that it's actually running on Amiga hardware.
 */
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

static int InitCustom(struct GfxBase *gfx)
{
	volatile struct Custom *custom = (struct Custom*)0xdff000;
	UWORD vposr, deniseid;
	UWORD flags = 0;
	UBYTE chipflags = 0;
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

   	Enable();

   	if (vposr >= 0x2200)
   		chipflags |= GFXF_AA_ALICE;

	gfx->DisplayFlags = flags;
	gfx->ChipRevBits0 = chipflags;

	SysBase->VBlankFrequency = (flags & PAL) ? 50 : 60;

	return TRUE;
}

/* This is run before the main init routine */
ADD2INITLIB(InitCustom, -5);

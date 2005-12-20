/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#define DEBUG 1
#include <proto/exec.h>
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/macros.h>
#include <exec/resident.h>

#include "bootmenu_intern.h"
#include "menu.h"
#include LC_LIBDEFS_FILE
#include <libraries/bootmenu.h>

#undef SysBase

#define LC_RESIDENTPRI -49
#define LC_LIBBASESIZE sizeof(LIBBASETYPE)
#define LC_RESIDENTFLAGS (RTF_COLDSTART | RTF_AUTOINIT)
#include <libcore/libheader.c>
#define bootmenubase ((LIBBASETYPEPTR)lh)

#define SysBase (((struct LibHeader *)bootmenubase)->lh_SysBase)

#warning "Argh! A functionless library doesn't get a function table!?"
VOID *const LIBFUNCTABLE[] =
{
	&LibHeader_BootmenuOpenLib,
	&LibHeader_BootmenuCloseLib,
	&LibHeader_BootmenuExpungeLib,
	(VOID *)-1
};

static struct BootConfig bootcfg =
{
	&bootcfg,
	{"vga.hidd", "hidd.gfx.vga"},
	{"kbd.hidd", "hidd.kbd.hw"},
	{"mouse.hidd", "hidd.bus.mouse"},
	NULL,
	TRUE
};

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh) {

	bootmenubase->GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	if (bootmenubase->GfxBase != NULL)
	{
		bootmenubase->IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
		if (bootmenubase->IntuitionBase != NULL)
		{
			CopyMem(&bootcfg, &bootmenubase->bcfg, sizeof(struct BootConfig));
			/* init keyboard + check */
			if (buttonsPressed(bootmenubase, &bootmenubase->bcfg.defaultkbd))
			{
				kprintf("Entering Boot Menu ...\n");
				/* init mouse + gfx */
				if (initHidds(&bootmenubase->bcfg, bootmenubase))
				{
					initScreen(bootmenubase, &bootmenubase->bcfg);
					return TRUE;
				}
			}
			else
				return TRUE;
		}
		CloseLibrary((struct Library *)bootmenubase->GfxBase);
	}
	return FALSE;
}

#undef GfxBase
#undef IntuitionBase
#define GfxBase bootmenubase->GfxBase
#define IntuitionBase bootmenubase->IntuitionBase

ULONG SAVEDS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh) {
	return TRUE;
}

VOID SAVEDS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh) {

	CloseLibrary((struct Library *)IntuitionBase);
	CloseLibrary((struct Library *)GfxBase);
}

VOID SAVEDS LC_BUILDNAME(L_CloseLib) (LC_LIBHEADERTYPEPTR lh) {
	return;
}


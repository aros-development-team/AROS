#include <aros/bootloader.h>
#include <libraries/bootmenu.h>
#include <proto/bootloader.h>
#include <string.h>

#include "bootmenu_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase)
{
    struct VesaInfo *vi;

    bootcfg->self = bootcfg;
    bootcfg->boot = NULL;
    strcpy(bootcfg->defaultgfx.libname,    "vgah.hidd");
    strcpy(bootcfg->defaultgfx.hiddname,   "hidd.gfx.vga");
    strcpy(bootcfg->defaultkbd.libname,    "kbd.hidd");
    strcpy(bootcfg->defaultkbd.hiddname,   "hidd.kbd.hw");
    strcpy(bootcfg->defaultmouse.libname,  "mouse.hidd");
    strcpy(bootcfg->defaultmouse.hiddname, "hidd.bus.mouse");

    if (!BootLoaderBase)
	return;

    if ((vi = (struct VesaInfo *)GetBootInfo(BL_Video)) != NULL)
    {
        if (vi->ModeNumber != 3)
        {
            strcpy(bootcfg->defaultgfx.libname, "vesagfx.hidd");
            strcpy(bootcfg->defaultgfx.hiddname, "hidd.gfx.vesa");
        }
    }
}

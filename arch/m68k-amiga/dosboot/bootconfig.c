#include <aros/bootloader.h>
#include <proto/bootloader.h>

#include "dosboot_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase)
{
    /* No video drivers yet! */
    bootcfg->gfxlib  = NULL; //"vgah.hidd";
    bootcfg->gfxhidd = NULL; //"hidd.gfx.vga";
    
    /* HIDD may need to be unloaded when another GFX driver is found */
    bootcfg->bootmode = TRUE;

	return;
}

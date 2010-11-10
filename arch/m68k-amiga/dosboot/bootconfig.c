#include <aros/bootloader.h>
#include <proto/bootloader.h>

#include "dosboot_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase)
{
    bootcfg->gfxlib  = "amigavideo.hidd";
    bootcfg->gfxhidd = "hidd.gfx.amigavideo";
    
    /* Boot driver may need to be unloaded when another GFX driver is found */
    bootcfg->bootmode = TRUE;
}

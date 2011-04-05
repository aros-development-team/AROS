#include <aros/bootloader.h>
#include <proto/bootloader.h>

#include "dosboot_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase)
{
    struct VesaInfo *vi;

    bootcfg->gfxlib  = "vgah.hidd";
    bootcfg->gfxhidd = "hidd.gfx.vga";
    
    /* VGA and VESA drivers need to be unloaded when another GFX driver is found */
    bootcfg->bootmode = TRUE;

    if (!BootLoaderBase)
	return;

    if ((vi = (struct VesaInfo *)GetBootInfo(BL_Video)) != NULL)
    {
        if (vi->ModeNumber != 3)
        {
            /* VESA driver is self-initing now */
            bootcfg->gfxhidd = NULL;
        }
    }
}

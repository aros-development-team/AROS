#include "dosboot_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase)
{
    bootcfg->gfxlib  = "radeon.hidd";
    bootcfg->gfxhidd = "RadeonDriver";
}

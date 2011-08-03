#include "dosboot_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg)
{
    bootcfg->gfxlib  = "radeon.hidd";
    bootcfg->gfxhidd = "RadeonDriver";
}

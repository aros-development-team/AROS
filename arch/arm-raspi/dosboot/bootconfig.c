#include "dosboot_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg)
{
    bootcfg->gfxlib  = "bcm2835.hidd";
    bootcfg->gfxhidd = "BCM2835Driver";
}

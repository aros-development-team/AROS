#include <string.h>

#include "dosboot_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase)
{
    strcpy(bootcfg->defaultgfx.libname,  "radeon.hidd");
    strcpy(bootcfg->defaultgfx.hiddname, "RadeonDriver");
    bootcfg->defaultkbd.hiddname[0]    = 0;
    bootcfg->defaultmouse.hiddname[0]  = 0;
}

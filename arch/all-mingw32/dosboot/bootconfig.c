#include <string.h>

#include "dosboot_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase)
{
    bootcfg->defaultgfx.hiddname[0] = 0;
    strcpy(bootcfg->defaultkbd.libname,    "wingdi.hidd");
    strcpy(bootcfg->defaultkbd.hiddname,   "hidd.kbd.gdi");
    strcpy(bootcfg->defaultmouse.libname,  "wingdi.hidd");
    strcpy(bootcfg->defaultmouse.hiddname, "hidd.mouse.gdi");
}

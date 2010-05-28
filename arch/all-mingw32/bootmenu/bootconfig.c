#include <libraries/bootmenu.h>
#include <proto/bootloader.h>
#include <string.h>

#include "bootmenu_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase)
{
    bootcfg->self = bootcfg;
    bootcfg->boot = NULL;
    bootcfg->defaultgfx.hiddname[0] = 0;
    strcpy(bootcfg->defaultkbd.libname,    "wingdi.hidd");
    strcpy(bootcfg->defaultkbd.hiddname,   "hidd.kbd.gdi");
    strcpy(bootcfg->defaultmouse.libname,  "wingdi.hidd");
    strcpy(bootcfg->defaultmouse.hiddname, "hidd.mouse.gdi");
}

#include <aros/bootloader.h>
#include <libraries/bootmenu.h>
#include <proto/bootloader.h>
#include <string.h>

#include "bootmenu_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase)
{
    bootcfg->self = bootcfg;
    bootcfg->boot = NULL;
    strcpy(bootcfg->defaultgfx.libname,  "radeon.hidd");
    strcpy(bootcfg->defaultgfx.hiddname, "RadeonDriver");
    bootcfg->defaultkbd.hiddname[0]    = 0;
    bootcfg->defaultmouse.hiddname[0]  = 0;
}

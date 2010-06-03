#include <aros/bootloader.h>
#include <string.h>

#include "dosboot_intern.h"

/* This file contains architecture-dependent defaults */

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase)
{
    /* TODO: We may have also framebuffer HIDD and we should choose it if X11
       isn't up and running. */
    strcpy(bootcfg->defaultgfx.libname,  "x11gfx.hidd");
    strcpy(bootcfg->defaultgfx.hiddname, "hidd.gfx.x11");
    strcpy(bootcfg->defaultkbd.libname,  "x11gfx.hidd");
    strcpy(bootcfg->defaultkbd.hiddname, "hidd.kbd.x11");
    strcpy(bootcfg->defaultmouse.libname, "x11gfx.hidd");
    strcpy(bootcfg->defaultmouse.hiddname, "hidd.mouse.x11");
}

#include <string.h>

#include "dosboot_intern.h"

/* This file contains architecture-dependent defaults.
   Our HIDDs don't need hepler code any more */

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase)
{
    bootcfg->defaultgfx.hiddname[0] = 0;
    bootcfg->defaultkbd.hiddname[0] = 0;
    bootcfg->defaultmouse.hiddname[0] = 0;
}

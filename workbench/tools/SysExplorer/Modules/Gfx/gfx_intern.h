#ifndef SYSEXPLORER_GFX_INTERN_H
#define SYSEXPLORER_GFX_INTERN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Private data for the SysExplorer graphics enumeration module.
*/

#include <exec/libraries.h>

#include <hidd/hidd.h>
#include <hidd/gfx.h>

#include "sysexp_intern.h"
#include "sysexp_module.h"

struct SysexpGfxBase
{
    struct Library              segb_Lib;
    struct SysexpBase           *segb_SysexpBase;
    struct SysexpModule         segb_Module;
    /**/
    struct MUI_CustomClass      *segb_GfxWindowCLASS;
    struct MUI_CustomClass      *segb_MonitorWindowCLASS;
    /**/
    struct MUI_CustomClass      *segb_DevicePageCLASS;
    struct MUI_CustomClass      *segb_GenericWindowCLASS;
};

#endif /* SYSEXPLORER_GFX_INTERN_H */

#ifndef PREFS_WORKBENCH_H
#define PREFS_WORKBENCH_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Workbench prefs definitions
    Lang: English
*/

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#ifndef GRAPHICS_GFX_H
#include <graphics/gfx.h>
#endif

#define ID_WBNC MAKE_ID('W','B','N','C')

struct WorkbenchPrefs {
    ULONG               wbp_DefaultStackSize;
    ULONG               wbp_TypeRestartTime;

    ULONG               wbp_IconPrecision;
    struct Rectangle    wbp_EmbossRect;
    BOOL                wbp_Borderless;
    LONG                wbp_MaxNameLength;
    BOOL                wbp_NewIconsSupport;
    BOOL                wbp_ColorIconSupport;
};

#define ID_WBHD MAKE_ID('W','B','H','D')

struct WorkbenchHiddenDevicePrefs {
    UBYTE   whdp_Name[0];
};

#endif /* PREFS_WORKBENCH_H */

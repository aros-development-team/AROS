/*
    Copyright © 2003-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SMPROPERTIES_H
#define _SMPROPERTIES_H

#include "smprefsbases.h"

#define MUIA_ScreenModeProperties_DisplayID     (MUIB_ScreenModeProperties + 1)
#define MUIA_ScreenModeProperties_Width         (MUIB_ScreenModeProperties + 2)
#define MUIA_ScreenModeProperties_Height        (MUIB_ScreenModeProperties + 3)
#define MUIA_ScreenModeProperties_Depth         (MUIB_ScreenModeProperties + 4)
#define MUIA_ScreenModeProperties_Autoscroll    (MUIB_ScreenModeProperties + 5)
#define MUIA_ScreenModeProperties_WidthString   (MUIB_ScreenModeProperties + 6)
#define MUIA_ScreenModeProperties_HeightString  (MUIB_ScreenModeProperties + 7)
#define MUIA_ScreenModeProperties_DefWidth      (MUIB_ScreenModeProperties + 8)
#define MUIA_ScreenModeProperties_DefHeight     (MUIB_ScreenModeProperties + 9)

extern struct MUI_CustomClass *ScreenModeProperties_CLASS;
#define ScreenModePropertiesObject BOOPSIOBJMACRO_START(ScreenModeProperties_CLASS->mcc_Class)

#endif


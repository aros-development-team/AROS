/*
    Copyright © 2003-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SMPROPERTIES_H
#define _SMPROPERTIES_H

#define MUIA_ScreenModeProperties_DisplayID     (TAG_USER | 1)
#define MUIA_ScreenModeProperties_Width         (TAG_USER | 2)
#define MUIA_ScreenModeProperties_Height        (TAG_USER | 3)
#define MUIA_ScreenModeProperties_Depth         (TAG_USER | 4)
#define MUIA_ScreenModeProperties_Autoscroll    (TAG_USER | 5)
#define MUIA_ScreenModeProperties_WidthString   (TAG_USER | 6)
#define MUIA_ScreenModeProperties_HeightString  (TAG_USER | 7)
#define MUIA_ScreenModeProperties_DefWidth      (TAG_USER | 8)
#define MUIA_ScreenModeProperties_DefHeight     (TAG_USER | 9)

extern struct MUI_CustomClass *ScreenModeProperties_CLASS;
#define ScreenModePropertiesObject BOOPSIOBJMACRO_START(ScreenModeProperties_CLASS->mcc_Class)

#endif


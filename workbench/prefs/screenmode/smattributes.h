/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SMATTRIBUTES_H
#define _SMATTRIBUTES_H

#include "smprefsbases.h"

#define MUIA_ScreenModeAttributes_DisplayID  (MUIB_ScreenModeAttributes + 1)

extern struct MUI_CustomClass *ScreenModeAttributes_CLASS;
#define ScreenModeAttributesObject BOOPSIOBJMACRO_START(ScreenModeAttributes_CLASS->mcc_Class)

#endif


/*
    Copyright © 2003-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SMSELECTOR_H
#define _SMSELECTOR_H

#include "smprefsbases.h"

#define MUIA_ScreenModeSelector_Active  (MUIB_ScreenModeSelector + 1)
#define MUIA_ScreenModeSelector_Mode    (MUIB_ScreenModeSelector + 2)

extern struct MUI_CustomClass *ScreenModeSelector_CLASS;
#define ScreenModeSelectorObject BOOPSIOBJMACRO_START(ScreenModeSelector_CLASS->mcc_Class)

#endif


/*
    Copyright � 2003-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SMSELECTOR_H
#define _SMSELECTOR_H

#define MUIA_ScreenModeSelector_Active (TAG_USER | 1)

extern struct MUI_CustomClass *ScreenModeSelector_CLASS;
#define ScreenModeSelectorObject BOOPSIOBJMACRO_START(ScreenModeSelector_CLASS->mcc_Class)

#endif


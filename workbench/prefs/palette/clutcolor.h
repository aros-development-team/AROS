/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _CLUTCOLOR_H
#define _CLUTCOLOR_H

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_CLUTColor                  (TAG_USER | 0x11000000)

#define MUIA_CLUTColor_Index            (MUIB_CLUTColor + 1)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *CLUTColor_CLASS;
#define CLUTColorObject BOOPSIOBJMACRO_START(CLUTColor_CLASS->mcc_Class)

#endif


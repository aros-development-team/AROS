#ifndef COMPUTERPAGE_CL_H
#define COMPUTERPAGE_CL_H

/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_PropertyWin               (TAG_USER | 0x00000000)

/*** Attributes ****************************************************************/
#define MUIA_PropertyWin_Object        (MUIB_PropertyWin | 0x00000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *ComputerWindow_CLASS;
extern struct MUI_CustomClass *GenericWindow_CLASS;
extern struct MUI_CustomClass *DevicePage_CLASS;
extern struct MUI_CustomClass *ATAPage_CLASS;

/*** Macros *****************************************************************/
#define DevicePageObject BOOPSIOBJMACRO_START(DevicePage_CLASS->mcc_Class)

#endif /* COMPUTERPAGE_CL_H */

#ifndef DEVICEPAGE_CL_H
#define DEVICEPAGE_CL_H

/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_DevicePage                 (TAG_USER | 0x10000000)

/*** Methods ****************************************************************/
#define MUIM_DevicePage_Update          (MUIB_DevicePage | 0x00000000)
struct  MUIP_DevicePage_Update
{
    STACKED ULONG MethodID;
    STACKED OOP_Object *device_obj;
};

/*** Variables **************************************************************/
extern struct MUI_CustomClass *DevicePage_CLASS;

/*** Macros *****************************************************************/
#define DevicePageObject BOOPSIOBJMACRO_START(DevicePage_CLASS->mcc_Class)

#endif /* DEVICEPAGE_CL_H */

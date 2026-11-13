/*
    Copyright (C) 2022, The AROS Development Team.
*/

#include <aros/debug.h>

#define __STORAGE_NOLIBBASE__

#include <proto/sysexp.h>
#include <proto/power.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <mui/NListtree_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <devices/newstyle.h>
#include <devices/ata.h>

#include "locale.h"
#include "enums.h"

#include "power_classes.h"
#include "power_intern.h"

#if (0) // TODO : Move into libbase
extern OOP_AttrBase HiddAttrBase;
extern OOP_AttrBase HWAttrBase;
extern OOP_AttrBase HiddPowerAB;

extern OOP_MethodID HWBase;
extern OOP_MethodID HiddPowerBase;
#endif

CONST_STRPTR powerwindowclass_name = "PowerWindow.Class";

CONST_STRPTR powerenumfunc_name = "PowerEnum.Func";
CONST_STRPTR devicepageclass_name = "DevicePage.Class";
CONST_STRPTR genericwindowclass_name = "GenericWindow.Class";

static void powerHWEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *tn)
{
    bug("[power.sysexp] Object @ 0x%p, TreeNode @ 0x%p\n", obj, tn);
//    HW_EnumDrivers(obj, &powerenum_hook, tn);
}

void PowerStartup(struct SysexpBase *SysexpBase)
{
    struct SysexpPowerBase *PowerBase;

    D(bug("[power.sysexp] %s(%p)\n", __func__, SysexpBase));

    PowerBase = GetBase("Power.Module");

    D(bug("[power.sysexp] %s: PowerBase @ %p\n", __func__, PowerBase));


    PowerBase->sesb_GenericWindowCLASS = GetBase(genericwindowclass_name);
    PowerBase->sesb_DevicePageCLASS = GetBase(devicepageclass_name);
    PowerWindow_CLASS->mcc_Class->cl_UserData = (IPTR)PowerBase;

    RegisterBase(powerenumfunc_name, powerHWEnum);
    RegisterBase(powerwindowclass_name, PowerWindow_CLASS);

    RegisterClassHandler(CLID_Hidd_Power, 90, PowerWindow_CLASS, powerHWEnum, NULL);
}

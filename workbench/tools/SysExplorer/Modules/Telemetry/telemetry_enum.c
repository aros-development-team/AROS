/*
    Copyright (C) 2025, The AROS Development Team.
*/

#include <aros/debug.h>

#include <proto/sysexp.h>
#include <proto/oop.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <mui/NListtree_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include "locale.h"
#include "enums.h"

#include "telemetry_classes.h"
#include "telemetry_intern.h"

CONST_STRPTR telemetrywindowclass_name = "TelemetryWindow.Class";
CONST_STRPTR devicepageclass_name = "DevicePage.Class";
CONST_STRPTR genericwindowclass_name = "GenericWindow.Class";

void TelemetryStartup(struct SysexpBase *SysexpBase)
{
    struct SysexpTelemetryBase *SeTelemetryBase;

    D(bug("[telemetry.sysexp] %s(%p)\n", __func__, SysexpBase));

    SeTelemetryBase = GetBase("Telemetry.Module");

    D(bug("[telemetry.sysexp] %s: Telemetry Module Base @ %p\n", __func__, SeTelemetryBase));

    SeTelemetryBase->sesb_GenericWindowCLASS = GetBase(genericwindowclass_name);
    SeTelemetryBase->sesb_DevicePageCLASS = GetBase(devicepageclass_name);
    TelemetryWindow_CLASS->mcc_Class->cl_UserData = (IPTR)SeTelemetryBase;

    RegisterBase(telemetrywindowclass_name, TelemetryWindow_CLASS);

    RegisterClassHandler(CLID_Hidd_Telemetry, 90, TelemetryWindow_CLASS, NULL, NULL);
}

#ifndef SYSEXPLORER_TELEMETRY_INTERN_H
#define SYSEXPLORER_TELEMETRY_INTERN_H

#include <exec/libraries.h>

#include <hidd/hidd.h>
#include <hidd/telemetry.h>

#include "sysexp_intern.h"
#include "sysexp_module.h"

struct SysexpTelemetryBase
{
    struct Library              sesb_Lib;
    struct SysexpBase           *sesb_SysexpBase;
    struct SysexpModule         sesb_Module;
    /**/
    struct List                 sesb_HandlerList;
    /**/
    struct MUI_CustomClass      *sesb_GenericWindowCLASS;
    struct MUI_CustomClass      *sesb_DevicePageCLASS;

    OOP_AttrBase                sesb_HiddAttrBase;
    OOP_AttrBase                sesb_HWAttrBase;
    OOP_AttrBase                sesb_HiddTelemetryAB;

    OOP_MethodID                sesb_HWBase;
    OOP_MethodID                sesb_HiddTelemetryBase;
};

#define __IHidd                 (SeTelemetryBase)->sesb_HiddAttrBase
#define __IHW                   (SeTelemetryBase)->sesb_HWAttrBase
#define __IHidd_Telemetry       (SeTelemetryBase)->sesb_HiddTelemetryAB

#define HWBase                  (SeTelemetryBase)->sesb_HWBase
#define HiddTelemetryBase       (SeTelemetryBase)->sesb_HiddTelemetryBase

#endif /* SYSEXPLORER_TELEMETRY_INTERN_H */

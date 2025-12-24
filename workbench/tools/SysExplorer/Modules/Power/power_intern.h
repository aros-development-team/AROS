#ifndef SYSEXPLORER_STORAGE_INTERN_H
#define SYSEXPLORER_STORAGE_INTERN_H

#include <exec/libraries.h>

#include <hidd/hidd.h>
#include <hidd/telemetry.h>
#include <hidd/power.h>

#include "sysexp_intern.h"
#include "sysexp_module.h"

struct SysexpPowerBase
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
    OOP_AttrBase                sesb_HiddPowerAB;

    OOP_MethodID                sesb_HWBase;
    OOP_MethodID                sesb_HiddTelemetryBase;
    OOP_MethodID                sesb_HiddPowerBase;
};

#define __IHidd                 (PowerBase)->sesb_HiddAttrBase
#define __IHW                   (PowerBase)->sesb_HWAttrBase
#define __IHidd_Power           (PowerBase)->sesb_HiddPowerAB
#define __IHidd_Telemetry       (PowerBase)->sesb_HiddTelemetryAB

#define HWBase                  (PowerBase)->sesb_HWBase
#define HiddPowerBase           (PowerBase)->sesb_HiddPowerBase
#define HiddTelemetryBase       (PowerBase)->sesb_HiddTelemetryBase

#endif /* SYSEXPLORER_STORAGE_INTERN_H */
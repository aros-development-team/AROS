/*
    Copyright (C) 2025, The AROS Development Team.
*/

#include <aros/debug.h>

#include <proto/sysexp.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include "telemetry_intern.h"

extern void TelemetryStartup(struct SysexpBase *);

static int telemetryenum_init(struct SysexpTelemetryBase *SeTelemetryBase)
{
    const struct OOP_ABDescr telemetry_abd[] =
    {
        {IID_Hidd                   , &HiddAttrBase         },
        {IID_HW                     , &HWAttrBase           },
        {IID_Hidd_Telemetry         , &HiddTelemetryAB      },
        {NULL                       , NULL                  }
    };
    D(bug("[telemetry.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(telemetry_abd);

    HWBase = OOP_GetMethodID(IID_HW, 0);
    HiddTelemetryBase = OOP_GetMethodID(IID_Hidd_Telemetry, 0);

    return 2;
}

ADD2INITLIB(telemetryenum_init, 10);

AROS_LH1(void, ModuleInit,
                AROS_LHA(void *, SysexpBase, A0),
                struct SysexpTelemetryBase *, SeTelemetryBase, 5, Telemetry
)
{
    AROS_LIBFUNC_INIT

    D(bug("[telemetry.sysexp] %s(%p)\n", __func__, SysexpBase));

    SeTelemetryBase->sesb_SysexpBase = SysexpBase;
    SeTelemetryBase->sesb_Module.sem_Node.ln_Name = "Telemetry.Module";
    SeTelemetryBase->sesb_Module.sem_Node.ln_Pri = 100;
    SeTelemetryBase->sesb_Module.sem_Startup = TelemetryStartup;
    RegisterModule(&SeTelemetryBase->sesb_Module, SeTelemetryBase);

    return;

    AROS_LIBFUNC_EXIT
}

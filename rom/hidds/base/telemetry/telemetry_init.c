/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#define __NOLIBBASE__

#include LC_LIBDEFS_FILE

static int Telemetry_Init(struct HiddTelemetryIntBase *telemetryBase)
{
    struct class_static_data    *base = &telemetryBase->hbi_csd;
    D(bug("[Telemetry] %s()\n", __func__));

    base->cs_UtilityBase = OpenLibrary("utility.library", 36);
    if (!base->cs_UtilityBase)
        return FALSE;

    HiddTelemetryAB = OOP_ObtainAttrBase(IID_Hidd_Telemetry);
    HiddThermalAB = OOP_ObtainAttrBase(IID_Hidd_Thermal);
    HiddFanAB = OOP_ObtainAttrBase(IID_Hidd_Fan);
    HiddThermalBase = OOP_GetMethodID(IID_Hidd_Thermal, 0);
    HiddFanBase = OOP_GetMethodID(IID_Hidd_Fan, 0);
    D(bug("[Telemetry] %s: HiddTelemetryAB %x @ 0x%p\n", __func__, HiddTelemetryAB, &HiddTelemetryAB);)

    return TRUE;
}

ADD2INITLIB(Telemetry_Init, 0)

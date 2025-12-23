/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#define __NOLIBBASE__

#include LC_LIBDEFS_FILE

static int Telemetry_Init(struct HiddTelemetryIntBase *telemetryBase)
{
    struct class_static_data    *base = &telemetryBase->hbi_csd;
    D(bug("[Telemetry] %s()\n", __func__));

    HiddTelemetryAB = OOP_ObtainAttrBase(IID_Hidd_Telemetry);
    D(bug("[Telemetry] %s: HiddTelemetryAB %x @ 0x%p\n", __func__, HiddTelemetryAB, &HiddTelemetryAB);)

    return TRUE;
}

ADD2INITLIB(Telemetry_Init, 0)
